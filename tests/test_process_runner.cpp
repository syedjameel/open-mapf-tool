#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include "ProcessRunner.h"

TEST_CASE("ProcessRunner captures stdout and exit code", "[process]")
{
    ProcessRunner runner;
    auto r = runner.run({"/bin/echo", "hello world"});

    CHECK(r.started);
    CHECK(r.exitCode == 0);
    CHECK(r.stdoutText.find("hello world") != std::string::npos);
    CHECK_FALSE(r.terminated);
}

TEST_CASE("ProcessRunner passes arguments verbatim (no shell interpretation)", "[process]")
{
    ProcessRunner runner;
    // With std::system, this argument would have been mangled by the shell.
    auto r = runner.run({"/bin/echo", "a b; rm -rf $HOME && `boom`"});

    CHECK(r.started);
    CHECK(r.exitCode == 0);
    CHECK(r.stdoutText.find("a b; rm -rf $HOME && `boom`") != std::string::npos);
}

TEST_CASE("ProcessRunner captures stderr and non-zero exit codes", "[process]")
{
    ProcessRunner runner;
    auto r = runner.run({"/bin/sh", "-c", "echo oops 1>&2; exit 3"});

    CHECK(r.started);
    CHECK(r.exitCode == 3);
    CHECK(r.stderrText.find("oops") != std::string::npos);
}

TEST_CASE("ProcessRunner reports launch failure for missing executables", "[process]")
{
    ProcessRunner runner;
    auto r = runner.run({"/nonexistent/binary"});

    CHECK_FALSE(r.started);
    CHECK_FALSE(r.errorMessage.empty());
}

TEST_CASE("ProcessRunner::terminate kills a running child", "[process]")
{
    ProcessRunner runner;
    ProcessRunner::Result result;

    auto start = std::chrono::steady_clock::now();
    std::thread worker([&] { result = runner.run({"/bin/sleep", "30"}); });

    // Give the child time to start, then kill it.
    while (!runner.isRunning() &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(5)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(runner.isRunning());
    runner.terminate();
    worker.join();
    auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(result.started);
    CHECK(result.terminated);
    // Must return far sooner than the child's 30s sleep.
    CHECK(elapsed < std::chrono::seconds(10));
}
