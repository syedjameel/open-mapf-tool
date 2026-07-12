#include "ProcessRunner.h"

#include <cstdio>

#include "subprocess.h"

namespace {

// Drain a FILE* completely into a string.
std::string readAll(std::FILE* f)
{
    std::string out;
    if (f == nullptr) {
        return out;
    }
    char buffer[4096];
    size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0) {
        out.append(buffer, n);
    }
    return out;
}

} // namespace

ProcessRunner::~ProcessRunner()
{
    terminate();
}

ProcessRunner::Result ProcessRunner::run(const std::vector<std::string>& argv)
{
    Result result;
    if (argv.empty()) {
        result.errorMessage = "no command given";
        return result;
    }

    // Build the NULL-terminated C argv expected by subprocess_create.
    std::vector<const char*> cargv;
    cargv.reserve(argv.size() + 1);
    for (const auto& a : argv) {
        cargv.push_back(a.c_str());
    }
    cargv.push_back(nullptr);

    auto process = new subprocess_s();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_terminateRequested = false;
        if (subprocess_create(cargv.data(),
                              subprocess_option_inherit_environment |
                                  subprocess_option_no_window,
                              process) != 0) {
            delete process;
            result.errorMessage = "failed to launch '" + argv[0] + "'";
            return result;
        }
        result.started = true;
        m_process = process;
    }

    int exitCode = -1;
    subprocess_join(process, &exitCode); // blocks until child exits/killed

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_process = nullptr;
        result.terminated = m_terminateRequested;
    }

    result.exitCode = exitCode;
    if (!result.terminated) {
        // Note: reading drains the pipes until every writer has closed
        // them. After a kill, orphaned grandchildren of the child may still
        // hold the pipe open (e.g. "sh -c 'solver ...'"), which would block
        // here for their whole lifetime -- and the output of a killed run
        // is meaningless anyway, so skip it.
        result.stdoutText = readAll(subprocess_stdout(process));
        result.stderrText = readAll(subprocess_stderr(process));
    }

    subprocess_destroy(process);
    delete process;
    return result;
}

void ProcessRunner::terminate()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_process == nullptr) {
        return;
    }
#if !defined(_WIN32)
    // subprocess_join zeroes the pid once the child is reaped;
    // kill(0, ...) would signal our own process group. Guard against the
    // tiny window between join returning and run() clearing m_process.
    if (m_process->child == 0) {
        return;
    }
#endif
    m_terminateRequested = true;
    subprocess_terminate(m_process);
}

bool ProcessRunner::isRunning() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_process != nullptr;
}

std::string ProcessRunner::joinArgv(const std::vector<std::string>& argv)
{
    std::string out;
    for (const auto& a : argv) {
        if (!out.empty()) {
            out += ' ';
        }
        out += a;
    }
    return out;
}

std::string ProcessRunner::excerpt(const std::string& text, std::size_t maxLen)
{
    if (text.size() <= maxLen) {
        return text;
    }
    return text.substr(0, maxLen) + "...";
}
