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
    result.stdoutText = readAll(subprocess_stdout(process));
    result.stderrText = readAll(subprocess_stderr(process));

    subprocess_destroy(process);
    delete process;
    return result;
}

void ProcessRunner::terminate()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_process != nullptr) {
        m_terminateRequested = true;
        subprocess_terminate(m_process);
    }
}

bool ProcessRunner::isRunning() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_process != nullptr;
}
