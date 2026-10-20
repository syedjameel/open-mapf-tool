#pragma once

#include <mutex>
#include <string>
#include <vector>

struct subprocess_s; // from vendored subprocess.h (implementation file only)

// Executes an external program directly (fork/exec — never through a shell,
// so arguments with spaces or shell metacharacters are safe), captures its
// stdout/stderr, and supports killing the child from another thread.
// Replaces the previous std::system() usage (shell
// injection/space breakage, and cancel() that could not stop the child).
class ProcessRunner
{
public:
    struct Result
    {
        bool started = false;    // false: program could not be launched
        bool terminated = false; // true: killed via terminate()
        int exitCode = -1;
        std::string stdoutText;
        std::string stderrText;
        std::string errorMessage; // set when started == false
    };

    ProcessRunner() = default;
    ~ProcessRunner();

    ProcessRunner(const ProcessRunner&) = delete;
    ProcessRunner& operator=(const ProcessRunner&) = delete;

    // Runs argv[0] with the given arguments and blocks until the child
    // exits (or is terminated). Intended to be called on a worker thread;
    // terminate() may be called concurrently from any other thread.
    Result run(const std::vector<std::string>& argv);

    // Kills the currently running child, if any (safe to call at any time).
    void terminate();

    bool isRunning() const;

    // Helpers for logging/diagnostics
    static std::string joinArgv(const std::vector<std::string>& argv);
    static std::string excerpt(const std::string& text, std::size_t maxLen = 500);

private:
    mutable std::mutex m_mutex;
    subprocess_s* m_process = nullptr; // non-null while a child is live
    bool m_terminateRequested = false;
};
