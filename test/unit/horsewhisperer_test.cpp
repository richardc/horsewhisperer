#include <horsewhisperer/horsewhisperer.h>
#include "../test.h"

namespace HW = HorseWhisperer;

void prepareGlobal() {
    // configure horsewhisperer
    HW::SetAppName("test-app");
    HW::DefineGlobalFlag<bool>("global-get", "a test flag", false, nullptr);
    HW::DefineGlobalFlag<int>("global-bad-flag", "a bad test flag",
                              false, nullptr);
}

void prepareAction(std::function<int(std::vector<std::string>)> f) {
    HW::DefineAction("test-action", 0, false, "no description", "no help", f);
    HW::DefineActionFlag<bool>("test-action", "action-get", "a test flag",
                               false, nullptr);
}

TEST_CASE("reset", "[reset]") {
    SECTION("Reset resets global flags") {
        prepareGlobal();
        HW::Reset();
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("global-get"),
                          HW::undefined_flag_error);
    }
}

TEST_CASE("global GetFlag", "[global getflag]") {
    HW::Reset();
    prepareGlobal();

    SECTION("it returns the default value of a unset flag") {
        REQUIRE(HW::GetFlag<bool>("global-get") == false);
    }

    SECTION("it throws an exception when trying to access and undefined flag") {
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-global-get"),
                          HW::undefined_flag_error);
    }
}

TEST_CASE("global SetFlag", "[global setflag]") {
    HW::Reset();
    prepareGlobal();

    SECTION("it sets the value of a flag") {
        HW::SetFlag<bool>("global-get", true);
        REQUIRE(HW::GetFlag<bool>("global-get") == true);
    }

    SECTION("it sets the value of an int option") {
        HW::DefineGlobalFlag<int>("global-int", "test", 1, nullptr);
        HW::SetFlag<int>("global-int", 42);
        REQUIRE(HW::GetFlag<int>("global-int") == 42);
    }

    SECTION("it sets the value of a double option") {
        HW::DefineGlobalFlag<double>("global-double", "test", 1.1, nullptr);
        HW::SetFlag<double>("global-double", 3.14);
        REQUIRE(HW::GetFlag<double>("global-double") == 3.14);
    }

    SECTION("it sets the value of a string option") {
        HW::DefineGlobalFlag<std::string>("global-string", "test", "bar", nullptr);
        HW::SetFlag<std::string>("global-string", "foo");
        REQUIRE(HW::GetFlag<std::string>("global-string") == "foo");
    }

    SECTION("it throws when trying to set an undefined flag") {
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-global-get", false),
                          HW::undefined_flag_error);
    }

    SECTION("it does not throws when flag validation succeeds") {
        HorseWhisperer::DefineGlobalFlag<bool>("global-success",
                                               "a test flag",
                                               false,
                                               [](bool) -> bool { return true; });
        REQUIRE_NOTHROW(HW::SetFlag<bool>("global-success", false));
    }

    SECTION("it throws when flag validation fails") {
        HorseWhisperer::DefineGlobalFlag<bool>("global-failure",
                                               "a test flag",
                                               false,
                                               [](bool) -> bool { return false; });
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("global-failure", false),
                         HW::flag_validation_error);
    }
}

TEST_CASE("GetFlagType", "[type]") {
    HW::Reset();
    prepareGlobal();

    SECTION("correctly gives Bool type for a flag") {
        HW::SetFlag<bool>("global-get", true);
        REQUIRE(HW::GetFlagType("global-get") == HW::FlagType::Bool);
    }

    SECTION("correctly gives Int type") {
        HW::DefineGlobalFlag<int>("global-int", "test", 1, nullptr);
        HW::SetFlag<int>("global-int", 42);
        REQUIRE(HW::GetFlagType("global-int") == HW::FlagType::Int);
    }

    SECTION("correctly gives Double type") {
        HW::DefineGlobalFlag<double>("global-double", "test", 1.1, nullptr);
        HW::SetFlag<double>("global-double", 3.14);
        REQUIRE(HW::GetFlagType("global-double") == HW::FlagType::Double);
    }

    SECTION("correctly gives string type") {
        HW::DefineGlobalFlag<std::string>("global-string", "test", "bar", nullptr);
        HW::SetFlag<std::string>("global-string", "foo");
        REQUIRE(HW::GetFlagType("global-string") == HW::FlagType::String);
    }

    SECTION("it throws when trying to set an undefined flag") {
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-global-get", false),
                          HW::undefined_flag_error);
    }

    SECTION("it does not throws when flag validation succeeds") {
        HorseWhisperer::DefineGlobalFlag<bool>("global-success",
                                               "a test flag",
                                               false,
                                               [](bool) -> bool { return true; });
        REQUIRE_NOTHROW(HW::SetFlag<bool>("global-success", false));
    }

    SECTION("it throws when flag validation fails") {
        HorseWhisperer::DefineGlobalFlag<bool>("global-failure",
                                               "a test flag",
                                               false,
                                               [](bool) -> bool { return false; });
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("global-failure", false),
                         HW::flag_validation_error);
    }
}

int getTest(std::vector<std::string>) {
    SECTION("it returns the default value of a unset flag") {
        // check local flag context
        REQUIRE(HW::GetFlag<bool>("action-get") == false);
        // check global flag context
        REQUIRE(HW::GetFlag<bool>("global-get") == false);
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-action-get"),
                          HW::undefined_flag_error);
    }
    return 0;
}

TEST_CASE("action GetFlag", "[action getflag]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(getTest);
    const char* action[] = { "test-app", "test-action" };

    // duck around Wc++11-compat-deprecated-writable-strings
    HW::Parse(2, const_cast<char**>(action));
    HW::Start();
}

int setTest(std::vector<std::string>) {
    SECTION("it set's an action specific flag") {
        HW::SetFlag<bool>("action-get", true);
        REQUIRE(HW::GetFlag<bool>("action-get") == true);
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-action-set", false),
                          HW::undefined_flag_error);
    }
    return 0;
}

TEST_CASE("action SetFlag", "[action setflag]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(setTest);
    const char* action[] = { "test-app", "test-action" };

    // duck around Wc++11-compat-deprecated-writable-strings
    HW::Parse(2, const_cast<char**>(action));
    HW::Start();
}

int testActionCallback(std::vector<std::string> args) {
    return 0;
}

TEST_CASE("parse", "[parse]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(nullptr);

    SECTION("returns PARSE_OK on success") {
        const char* args[] = { "test-app", "test-action"};
        REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::PARSE_OK);
    }

    SECTION("returns PARSE_HELP on the help flag") {
        const char* args[] = { "test-app", "test-action", "--help"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_HELP);
    }

    SECTION("returns PARSE_VERSION on the --version flag") {
        const char* args[] = { "test-app", "test-action", "--version"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_VERSION);
    }

    SECTION("returns PARSE_EERROR on bad arguments") {
        const char* args[] = { "test-app", "test-action", "test-smachtions"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_ERROR);
    }

    SECTION("returns PARSE_INVALID_FLAG on a bad flag") {
        const char* args[] = { "test-app", "test-action", "--global-bad-flag", "foo" };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::PARSE_INVALID_FLAG);
    }

    SECTION("returns PARSE_OK when mixing key=value and other flags") {
        HW::DefineGlobalFlag<int>("foo", "a int test flag", 0, nullptr);
        HW::DefineGlobalFlag<bool>("bar", "a bool test flag", false, nullptr);

        const char* args[] = { "test-app", "test-action", "--bar", "--foo=5" };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::PARSE_OK);
    }

    SECTION("returns PARSE_OK when flag is given as key=value") {
        HW::DefineGlobalFlag<int>("foo", "a test flag", 0, nullptr);
        const char* args[] = { "test-app", "test-action", "--foo=5" };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_OK);
    }

    HW::DefineAction("new_action", 2, false, "test action", "2 args required!",
                     testActionCallback);

    SECTION("returns PARSE_ERROR if two action arguments are missing") {
        const char* args[] = { "test-app", "new_action" };
        REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::PARSE_ERROR);
    }

    SECTION("return PARSE_ERROR if one action argument is missing") {
        const char* args[] = { "test-app", "new_action", "spam" };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_ERROR);
    }

    SECTION("returns PARSE_ERROR if one action arguments is missing and a "
            "flag is passed") {
        const char* args[] = { "test-app", "new_action", "spam", "--verbose" };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::PARSE_ERROR);
    }

    SECTION("returns PARSE_OK if all action arguments are provided") {
        const char* args[] = { "test-app", "new_action", "spam", "eggs" };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::PARSE_OK);
    }
}

auto action_callback = [](std::vector<std::string>) -> int { return 0; };

TEST_CASE("HorseWhisperer::getActions" "[getActions]") {
    HW::Reset();
    prepareGlobal();
    HorseWhisperer::DefineAction("new_action", 2, true, "no description",
                                 "no help", action_callback);

    SECTION("returns a vector containing a single action name") {
        const char* args[] = { "test-app", "new_action", "spam", "eggs" };
        std::vector<std::string> test_result { "new_action" };
        HW::Parse(4, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    HorseWhisperer::DefineAction("new_action_2", 0, true, "no description",
                                  "no help", action_callback);

    SECTION("returns multiple action names") {
        const char* args[] = { "test-app", "new_action", "spam", "eggs",
                               "new_action_2" };
        std::vector<std::string> test_result { "new_action", "new_action_2" };
        HW::Parse(5, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    std::vector<std::string> delim { "+" };
    HW::SetDelimiters(delim);

    SECTION("works properly with user-defined delimiters") {
        const char* args[] = { "test-app", "new_action", "foo", "bar",
                               "+", "new_action_2" };
        std::vector<std::string> test_result { "new_action", "new_action_2" };
        HW::Parse(6, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    SECTION("returns duplicate actions") {
        const char* args[] = { "test-app", "new_action", "foo", "bar",
                               "+", "new_action_2",
                               "+", "new_action", "spam", "eggs" };
        std::vector<std::string> test_result { "new_action", "new_action_2",
                                               "new_action" };
        HW::Parse(10, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }
}

TEST_CASE("HorseWhisperer::Start", "[start]") {
    HW::Reset();
    prepareGlobal();

    SECTION("it executes an action") {
        int modify_me = 0;
        HW::DefineAction("start_test_1", 0, false, "test-action", "no help",
                         [&modify_me](std::vector<std::string>) -> int {
                            return ++modify_me; });

        const char* cli[] = { "test-app", "start_test_1" };
        HW::Parse(2, const_cast<char**>(cli));
        HW::Start();
        REQUIRE(modify_me == 1);
    }

    SECTION("it can chain actions") {
        int modify_me1 = 0;
        int modify_me2 = 1;
        std::vector<std::string> delim { "+" };
        HW::SetDelimiters(delim);

        HW::DefineAction("chain_test_1", 0, true, "test-action", "no help",
                         [&modify_me1](std::vector<std::string>) -> int {
                            ++modify_me1; return 0; });

        HW::DefineAction("chain_test_2", 0, true, "test-action", "no help",
                         [&modify_me2](std::vector<std::string>) -> int {
                            ++modify_me2; return 0; });

        const char* cli[] = { "test-app", "chain_test_1", "+", "chain_test_2" };
        HW::Parse(4, const_cast<char**>(cli));
        HW::Start();
        REQUIRE(modify_me1 == 1);
        REQUIRE(modify_me2 == 2);
    }

    SECTION("chained actions have confined flags and arguments") {
        int call_counter = 0;

        auto a_c = [&call_counter](std::vector<std::string> args) -> int {
            REQUIRE(args.size() == 1);
            auto t_v = HW::GetFlag<std::string>("test_flag");
            if (call_counter == 0) {
                REQUIRE(args[0] == "arg_one");
                REQUIRE(t_v == "spam");
            } else if (call_counter == 1) {
                REQUIRE(args[0] == "arg_two");
                REQUIRE(t_v == "eggs");
            } else if (call_counter == 2) {
                REQUIRE(args[0] == "arg_three");
                REQUIRE(t_v == "beans");
            } else {
                REQUIRE(false);
            }
            call_counter++;
            return 0;
        };

        HW::DefineAction("chain_test_3", 1, true, "test-action", "no help", a_c);
        HW::DefineActionFlag<std::string>("chain_test_3", "test_flag",
                                          "no description", "foo", nullptr);

        const char* cli[] = { "test-app",
                              "chain_test_3", "arg_one", "--test_flag", "spam",
                              "chain_test_3", "arg_two", "--test_flag", "eggs",
                              "chain_test_3", "arg_three", "--test_flag", "beans" };

        HW::Parse(13, const_cast<char**>(cli));
        HW::Start();
        REQUIRE(call_counter == 3);
    }
}
