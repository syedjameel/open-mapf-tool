// Integration smoke tests: point the drivers at *fake solver* shell scripts
// that emit canned output, exercising the whole generate -> execute -> parse
// pipeline headless, without any real solver installed. (Plan 5.1)
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <fstream>
#include <future>
#include <sstream>
#include <thread>
#include <vector>

#include "CBSDriver.h"
#include "LacamDriver.h"
#include "TestHelpers.h"

namespace {

std::vector<Agent> twoAgents()
{
    std::vector<Agent> agents;
    agents.emplace_back(0, CoordUtils::make_coord(0, 0), CoordUtils::make_coord(3, 3));
    agents.emplace_back(1, CoordUtils::make_coord(1, 0), CoordUtils::make_coord(2, 2));
    return agents;
}

std::string readFile(const std::string& path)
{
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

TEST_CASE("CBSDriver runs a fake solver end to end", "[solver][integration]")
{
    // Fake CBS: extracts -o and --outputPaths=..., writes a canned results
    // CSV and a canned paths file.
    TempScript fakeCbs("#!/bin/sh\n"
                       "out=\"\"; paths=\"\"\n"
                       "while [ $# -gt 0 ]; do\n"
                       "  case \"$1\" in\n"
                       "    -o) out=\"$2\"; shift 2;;\n"
                       "    --outputPaths=*) paths=\"${1#--outputPaths=}\"; shift;;\n"
                       "    *) shift;;\n"
                       "  esac\n"
                       "done\n"
                       "echo 'runtime,hl expanded,hl generated,ll expanded,ll generated,solution "
                       "cost' > \"$out\"\n"
                       "echo '0.5,10,20,30,40,7' >> \"$out\"\n"
                       "printf 'Agent 0: (0,0)->(1,1)\\nAgent 1: (1,0)->(2,2)\\n' > \"$paths\"\n");

    TempDir tempDir;
    Environment env;

    CBSDriver driver;
    driver.setExecutablePath(fakeCbs.path());

    std::promise<CBSResults> promise;
    auto fut = promise.get_future();
    driver.solveAsync(
        twoAgents(), env, CBSParameters{}, "itest",
        [&promise](const CBSResults& r) { promise.set_value(r); }, tempDir.path());

    REQUIRE(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
    CBSResults r = fut.get();

    INFO("error: " << r.errorMessage);
    CHECK(r.solutionFound);
    CHECK(r.solutionCost == 7);
    CHECK(r.runtime == 0.5);
    CHECK(r.highLevelNodes == 10);
    CHECK(r.lowLevelNodes == 30);
    REQUIRE_FALSE(r.pathsFile.empty());
    CHECK(readFile(r.pathsFile).find("Agent 0: (0,0)->(1,1)") != std::string::npos);
}

TEST_CASE("LacamDriver runs a fake solver end to end", "[solver][integration]")
{
    // Fake LaCAM: extracts -o, writes a canned LaCAM output log including a
    // solution section (per-timestep positions for all agents).
    TempScript fakeLacam("#!/bin/sh\n"
                         "out=\"\"\n"
                         "while [ $# -gt 0 ]; do\n"
                         "  case \"$1\" in\n"
                         "    -o) out=\"$2\"; shift 2;;\n"
                         "    *) shift;;\n"
                         "  esac\n"
                         "done\n"
                         "cat > \"$out\" <<'EOF'\n"
                         "solved=1\n"
                         "soc=12\n"
                         "soc_lb=10\n"
                         "makespan=2\n"
                         "makespan_lb=2\n"
                         "comp_time=250\n"
                         "solution=\n"
                         "0:(0,0),(1,0),\n"
                         "1:(1,1),(2,1),\n"
                         "2:(2,2),(2,1),\n"
                         "EOF\n");

    TempDir tempDir;
    Environment env;

    LacamDriver driver;
    driver.setExecutablePath(fakeLacam.path());

    std::promise<LacamResults> promise;
    auto fut = promise.get_future();
    driver.solveAsync(
        twoAgents(), env, LacamParameters{}, "itest",
        [&promise](const LacamResults& r) { promise.set_value(r); }, tempDir.path());

    REQUIRE(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
    LacamResults r = fut.get();

    INFO("error: " << r.errorMessage);
    CHECK(r.solutionFound);
    CHECK(r.solutionCost == 12);
    CHECK(r.makespan == 2);
    CHECK(r.runtime == 0.25); // comp_time is in milliseconds
    REQUIRE_FALSE(r.pathsFile.empty());

    // Timesteps 0..2 for agent 0: (0,0) -> (1,1) -> (2,2)
    std::string paths = readFile(r.pathsFile);
    CHECK(paths.find("Agent 0: (0,0)->(1,1)->(2,2)") != std::string::npos);
    CHECK(paths.find("Agent 1: (1,0)->(2,1)->(2,1)") != std::string::npos);
}

TEST_CASE("cancel() kills a hung solver promptly", "[solver][integration]")
{
    // Fake solver that would block for 60 seconds.
    TempScript hungSolver("#!/bin/sh\nsleep 60\n");

    TempDir tempDir;
    Environment env;

    CBSDriver driver;
    driver.setExecutablePath(hungSolver.path());

    std::atomic<bool> callbackInvoked{false};
    driver.solveAsync(
        twoAgents(), env, CBSParameters{}, "itest",
        [&callbackInvoked](const CBSResults&) { callbackInvoked = true; }, tempDir.path());

    // Wait until the solver thread is running, then cancel.
    auto start = std::chrono::steady_clock::now();
    while (!driver.isRunning() &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(driver.isRunning());
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // let the child start
    driver.cancel();

    // Previously cancel() only set a flag while std::system blocked for the
    // full child runtime. With ProcessRunner the child is killed, so the
    // driver must go idle far sooner than the 60s sleep.
    while (driver.isRunning() &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    CHECK_FALSE(driver.isRunning());
    CHECK_FALSE(callbackInvoked.load()); // cancelled runs do not report results
}
