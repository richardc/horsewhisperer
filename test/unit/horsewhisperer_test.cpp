#include <horsewhisperer/horsewhisperer.h>
#include "../test.h"

namespace HW = HorseWhisperer;

void prepareGlobal() {
    // configure horsewhisperer
    HorseWhisperer::SetAppName("test-app");
    HorseWhisperer::DefineGlobalFlag<bool>("global-get", "a test flag", false, nullptr);
    HorseWhisperer::DefineGlobalFlag<int>("global-bad-flag", "a bad test flag", false, nullptr);
}

void prepareAction(std::function<int(std::vector<std::string>)> f) {
    HorseWhisperer::DefineAction("test-action", 0, false, "test-action", "no help",
                                  f);
    HorseWhisperer::DefineActionFlag<bool>("test-action", "action-get",
                                           "a test flag", false, nullptr);
}

TEST_CASE("reset", "[reset") {
    SECTION("Reset resets global flags") {
        prepareGlobal();
        HW::Reset();
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("global-get"), HW::undefined_flag_error);
    }
}

TEST_CASE("global GetFlag", "[global getflag]") {
    HW::Reset();
    prepareGlobal();
    SECTION("it returns the default value of a unset flag") {
        REQUIRE(HW::GetFlag<bool>("global-get") == false);
    }
    SECTION("it throws an exception when trying to access and undefined flag") {
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-global-get"), HW::undefined_flag_error);
    }
}

TEST_CASE("global SetFlag", "[global setflag]") {
    HW::Reset();
    prepareGlobal();
    SECTION("it sets the value of a flag") {
        HW::SetFlag<bool>("global-get", true);
        REQUIRE(HW::GetFlag<bool>("global-get") == true);
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
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-action-get"), HW::undefined_flag_error);
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
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-action-set", false), HW::undefined_flag_error);
    }
    return 0;
}

TEST_CASE("action SetFlag", "[action setflag]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(setTest);
    const char* action[] = { "test-app", "test-action" };

    HorseWhisperer::DefineAction("test-action", 0, false, "test-action", "no help",
                                  setTest);
    HorseWhisperer::DefineActionFlag<bool>("test-action", "action-get",
                                           "a test flag", false, nullptr);
    // duck around Wc++11-compat-deprecated-writable-strings
    HW::Parse(2, const_cast<char**>(action));
    HW::Start();
}


TEST_CASE("parse", "[parse]") {
    HW::Reset();
    prepareGlobal();
    SECTION("it return PARSE_OK on success") {
        const char* args[] = { "test-app", "test-action"};
        REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::PARSE_OK);
    }

    SECTION("it return PARSE_HELP on the help flag") {
        const char* args[] = { "test-app", "test-action", "--help"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_HELP);
    }

    SECTION("it return PARSE_VERSION on the --version flag") {
        const char* args[] = { "test-app", "test-action", "--version"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_VERSION);
    }

    SECTION("it return PARSE_EERROR on bad arguments") {
        const char* args[] = { "test-app", "test-action", "test-smachtions"};
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::PARSE_ERROR);
    }

    SECTION("it return PARSE_INVALID_FLAG on a bad flag") {
        const char* args[] = { "test-app", "test-action", "--global-bad-flag", "foo" };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::PARSE_INVALID_FLAG);
    }
}

TEST_CASE("HorseWhisperer::Start", "[start]") {
    HW::Reset();
    prepareGlobal();
    SECTION("it executes an action") {
        HW::Reset();
        int modify_me = 0;
        HorseWhisperer::DefineAction("start_test_1", 0, false, "test-action", "no help",
                                     [&modify_me](std::vector<std::string>) -> int
                                        { return ++modify_me; });
        const char* cli[] = { "test-app", "start_test_1" };
        HW::Parse(2, const_cast<char**>(cli));
        HW::Start();
        REQUIRE(modify_me == 1);
    }

    SECTION("it can chain actions") {
        HW::Reset();

        int modify_me1 = 0;
        int modify_me2 = 1;
        std::vector<std::string> delim { "+" };
        HorseWhisperer::SetDelimiters(delim);

        HorseWhisperer::DefineAction("chain_test_1", 0, true, "test-action", "no help",
                                     [&modify_me1](std::vector<std::string>) -> int
                                        { ++modify_me1; return 0;});

        HorseWhisperer::DefineAction("chain_test_2", 0, true, "test-action", "no help",
                                     [&modify_me2](std::vector<std::string>) -> int
                                        { ++modify_me2; return 0;});

        const char* cli[] = { "test-app", "chain_test_1", "+", "chain_test_2" };
        HW::Parse(4, const_cast<char**>(cli));
        HW::Start();
        REQUIRE(modify_me1 == 1);
        REQUIRE(modify_me2 == 2);
    }
}
