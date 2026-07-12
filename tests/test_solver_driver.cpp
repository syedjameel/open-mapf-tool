#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <string>
#include <vector>

#include "CBSDriver.h"
#include "LacamDriver.h"

namespace {

std::vector<Agent> oneAgent()
{
    std::vector<Agent> agents;
    agents.emplace_back(0, CoordUtils::make_coord(0, 0), CoordUtils::make_coord(1, 1));
    return agents;
}

template <typename Results>
Results waitForResult(std::future<Results>& fut)
{
    REQUIRE(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
    return fut.get();
}

} // namespace

TEST_CASE("CBSDriver degrades gracefully without a solver binary", "[solver]")
{
    Environment env; // built-in 10x10 default grid

    SECTION("unconfigured solver reports a clear error")
    {
        CBSDriver driver; // executable path left empty
        std::promise<CBSResults> promise;
        auto fut = promise.get_future();

        driver.solveAsync(oneAgent(), env, CBSParameters{}, "testmap",
                          [&promise](const CBSResults& r) { promise.set_value(r); });

        CBSResults r = waitForResult(fut);
        CHECK_FALSE(r.solutionFound);
        CHECK(r.errorMessage.find("not configured") != std::string::npos);
    }

    SECTION("configured but missing executable reports a clear error")
    {
        CBSDriver driver;
        driver.setExecutablePath("/nonexistent/cbs");
        std::promise<CBSResults> promise;
        auto fut = promise.get_future();

        driver.solveAsync(oneAgent(), env, CBSParameters{}, "testmap",
                          [&promise](const CBSResults& r) { promise.set_value(r); });

        CBSResults r = waitForResult(fut);
        CHECK_FALSE(r.solutionFound);
        CHECK(r.errorMessage.find("not found") != std::string::npos);
        CHECK(r.errorMessage.find("/nonexistent/cbs") != std::string::npos);
    }
}

TEST_CASE("LacamDriver degrades gracefully without a solver binary", "[solver]")
{
    Environment env;

    SECTION("unconfigured solver reports a clear error")
    {
        Lacam2Driver driver; // executable path left empty
        std::promise<LacamResults> promise;
        auto fut = promise.get_future();

        driver.solveAsync(oneAgent(), env, LacamParameters{}, "testmap",
                          [&promise](const LacamResults& r) { promise.set_value(r); });

        LacamResults r = waitForResult(fut);
        CHECK_FALSE(r.solutionFound);
        CHECK(r.errorMessage.find("LaCAM2") != std::string::npos);
        CHECK(r.errorMessage.find("not configured") != std::string::npos);
        CHECK(r.errorMessage.find("lacam2") != std::string::npos); // config key hint
    }

    SECTION("configured but missing executable reports a clear error")
    {
        LacamDriver driver;
        driver.setExecutablePath("/nonexistent/lacam/main");
        std::promise<LacamResults> promise;
        auto fut = promise.get_future();

        driver.solveAsync(oneAgent(), env, LacamParameters{}, "testmap",
                          [&promise](const LacamResults& r) { promise.set_value(r); });

        LacamResults r = waitForResult(fut);
        CHECK_FALSE(r.solutionFound);
        CHECK(r.errorMessage.find("not found") != std::string::npos);
    }
}
