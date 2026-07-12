// Fixture-based tests for the solver output parsers (plan 5.1, priority 2).
// Fixtures live in tests/fixtures/ and follow the exact formats produced by
// CBSH2-RTC (results CSV) and LaCAM (key=value log with a solution section).
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <fstream>
#include <sstream>

#include "CBSDriver.h"
#include "LacamDriver.h"

#ifndef OPENMAPF_TEST_FIXTURES
#error "OPENMAPF_TEST_FIXTURES must be defined by the build"
#endif

namespace {

std::ifstream openFixture(const std::string& name)
{
    std::ifstream in(std::string(OPENMAPF_TEST_FIXTURES) + "/" + name);
    REQUIRE(in.is_open());
    return in;
}

} // namespace

TEST_CASE("CBS results CSV: solved run", "[parsers][cbs]")
{
    auto in = openFixture("cbs-results-good.csv");
    CBSResults r;
    REQUIRE(CBSDriver::parseResultsCsv(in, r));

    CHECK(r.solutionFound);
    CHECK(r.solutionCost == 55);
    CHECK_THAT(r.runtime, Catch::Matchers::WithinAbs(0.051269, 1e-9));
    CHECK(r.highLevelNodes == 12);
    CHECK(r.lowLevelNodes == 340);
}

TEST_CASE("CBS results CSV: unsolved run (cost -1)", "[parsers][cbs]")
{
    auto in = openFixture("cbs-results-unsolved.csv");
    CBSResults r;
    REQUIRE(CBSDriver::parseResultsCsv(in, r)); // row parses fine...
    CHECK_FALSE(r.solutionFound);               // ...but reports no solution
    CHECK(r.solutionCost == -1);
}

TEST_CASE("CBS results CSV: truncated and empty files fail cleanly", "[parsers][cbs]")
{
    SECTION("header only")
    {
        auto in = openFixture("cbs-results-truncated.csv");
        CBSResults r;
        CHECK_FALSE(CBSDriver::parseResultsCsv(in, r));
    }

    SECTION("empty file")
    {
        auto in = openFixture("cbs-results-empty.csv");
        CBSResults r;
        CHECK_FALSE(CBSDriver::parseResultsCsv(in, r));
    }

    SECTION("garbage row")
    {
        std::istringstream in("header\nnot,enough,fields\n");
        CBSResults r;
        CHECK_FALSE(CBSDriver::parseResultsCsv(in, r));
    }
}

TEST_CASE("LaCAM output: solved run with all metrics", "[parsers][lacam]")
{
    auto in = openFixture("lacam-output-good.txt");
    LacamResults r;
    REQUIRE(LacamDriverBase::parseLacamOutput(in, r));

    CHECK(r.solutionFound);
    CHECK(r.solutionCost == 12);
    CHECK(r.socLowerBound == 10);
    CHECK(r.makespan == 2);
    CHECK(r.makespanLowerBound == 2);
    CHECK(r.sumOfLoss == 12);
    CHECK(r.sumOfLossLowerBound == 10);
    CHECK_THAT(r.runtime, Catch::Matchers::WithinAbs(0.250, 1e-9)); // ms -> s
}

TEST_CASE("LaCAM output: solved=0 reports failure but keeps metrics", "[parsers][lacam]")
{
    auto in = openFixture("lacam-output-unsolved.txt");
    LacamResults r;
    CHECK_FALSE(LacamDriverBase::parseLacamOutput(in, r));
    CHECK_FALSE(r.solutionFound);
    CHECK_THAT(r.runtime, Catch::Matchers::WithinAbs(10.0, 1e-9));
}

TEST_CASE("LaCAM solution section converts to per-agent arrow paths", "[parsers][lacam]")
{
    auto in = openFixture("lacam-output-good.txt");
    std::ostringstream out;
    REQUIRE(LacamDriverBase::convertLacamPaths(in, out));

    const std::string paths = out.str();
    CHECK(paths.find("Agent 0: (0,0)->(1,0)->(1,1)") != std::string::npos);
    CHECK(paths.find("Agent 1: (1,1)->(2,1)->(2,2)") != std::string::npos);
    CHECK(paths.find("Agent 2: (2,2)->(2,3)->(2,1)") != std::string::npos);
}

TEST_CASE("LaCAM truncated solution converts the complete prefix", "[parsers][lacam]")
{
    auto in = openFixture("lacam-output-truncated.txt");
    std::ostringstream out;
    REQUIRE(LacamDriverBase::convertLacamPaths(in, out));

    const std::string paths = out.str();
    // Timestep 1 was cut mid-line after agent 0's position; agents 1 and 2
    // are padded with their last known position.
    CHECK(paths.find("Agent 0: (0,0)->(1,0)") != std::string::npos);
    CHECK(paths.find("Agent 1: (1,1)->(1,1)") != std::string::npos);
    CHECK(paths.find("Agent 2: (2,2)->(2,2)") != std::string::npos);
}

TEST_CASE("LaCAM output with no solution section fails conversion", "[parsers][lacam]")
{
    auto in = openFixture("lacam-output-unsolved.txt");
    std::ostringstream out;
    CHECK_FALSE(LacamDriverBase::convertLacamPaths(in, out));
    CHECK(out.str().empty());
}
