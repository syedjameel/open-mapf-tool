#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "ConfigLocator.h"
#include "Environment.h"
#include "TestHelpers.h"

namespace {

ConfigLocator::Inputs fullInputs()
{
    ConfigLocator::Inputs in;
    in.cliPath = "/cli/env.yaml";
    in.envPath = "/envvar/env.yaml";
    in.xdgConfigHome = "/xdg";
    in.homeDir = "/home/user";
    in.exeDir = "/opt/openmapf/bin";
    return in;
}

// exists-predicate that accepts exactly one path
auto onlyPath(const std::string& match)
{
    return [match](const std::string& p) { return p == match; };
}

} // namespace

TEST_CASE("candidatePaths lists all sources in priority order", "[config]")
{
    auto candidates = ConfigLocator::candidatePaths(fullInputs());

    REQUIRE(candidates.size() == 6);
    CHECK(candidates[0] == "/cli/env.yaml");
    CHECK(candidates[1] == "/envvar/env.yaml");
    CHECK(candidates[2] == "/xdg/openmapf/env.yaml");
    CHECK(candidates[3] == "/opt/openmapf/bin/../share/openmapf/env.yaml");
    CHECK(candidates[4] == "/opt/openmapf/bin/../config/env.yaml");
    CHECK(candidates[5] == "/opt/openmapf/bin/config/env.yaml");
}

TEST_CASE("XDG default falls back to $HOME/.config", "[config]")
{
    auto in = fullInputs();
    in.xdgConfigHome = "";

    auto candidates = ConfigLocator::candidatePaths(in);
    CHECK(std::find(candidates.begin(), candidates.end(),
                    "/home/user/.config/openmapf/env.yaml") != candidates.end());
}

TEST_CASE("unset sources are skipped entirely", "[config]")
{
    ConfigLocator::Inputs in; // everything empty
    CHECK(ConfigLocator::candidatePaths(in).empty());
    CHECK_FALSE(ConfigLocator::resolve(in, [](const std::string&) { return true; }).has_value());
}

TEST_CASE("resolve returns the first existing candidate", "[config]")
{
    auto in = fullInputs();

    SECTION("CLI path wins when present")
    {
        auto r = ConfigLocator::resolve(in, [](const std::string&) { return true; });
        REQUIRE(r.has_value());
        CHECK(*r == "/cli/env.yaml");
    }

    SECTION("environment variable wins when CLI file is absent")
    {
        auto r = ConfigLocator::resolve(in, onlyPath("/envvar/env.yaml"));
        REQUIRE(r.has_value());
        CHECK(*r == "/envvar/env.yaml");
    }

    SECTION("XDG path wins when CLI and env files are absent")
    {
        auto r = ConfigLocator::resolve(in, onlyPath("/xdg/openmapf/env.yaml"));
        REQUIRE(r.has_value());
        CHECK(*r == "/xdg/openmapf/env.yaml");
    }

    SECTION("exe-relative share path is used as fallback")
    {
        auto r = ConfigLocator::resolve(in, onlyPath("/opt/openmapf/bin/../share/openmapf/env.yaml"));
        REQUIRE(r.has_value());
        CHECK(*r == "/opt/openmapf/bin/../share/openmapf/env.yaml");
    }

    SECTION("nothing exists -> nullopt (caller uses built-in defaults)")
    {
        CHECK_FALSE(ConfigLocator::resolve(in, [](const std::string&) { return false; }).has_value());
    }
}

TEST_CASE("Environment::loadConfig degrades gracefully", "[config]")
{
    SECTION("missing file keeps defaults and returns false")
    {
        Environment env;
        CHECK_FALSE(env.loadConfig("/nonexistent/env.yaml"));
        CHECK(env.m_rows == 10); // built-in default
        CHECK(env.m_cols == 10);
    }

    SECTION("malformed YAML keeps defaults and returns false")
    {
        TempFile bad("params: [unbalanced", ".yaml");
        Environment env;
        CHECK_FALSE(env.loadConfig(bad.path()));
        CHECK(env.m_rows == 10);
    }

    SECTION("valid config is applied")
    {
        TempFile good(
            "params:\n"
            "  width: 800\n"
            "  height: 600\n"
            "  square: true\n"
            "  hexagon: false\n"
            "  rows: 7\n"
            "  cols: 9\n",
            ".yaml");
        Environment env;
        CHECK(env.loadConfig(good.path()));
        CHECK(env.m_rows == 7);
        CHECK(env.m_cols == 9);
        CHECK(env.m_width == 800);
    }

    SECTION("constructor with unreadable path must not throw")
    {
        CHECK_NOTHROW(Environment("/nonexistent/env.yaml"));
    }
}
