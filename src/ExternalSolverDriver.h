#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Environment.h"
#include "ISolutionParser.h"
#include "ProcessRunner.h"
#include "Types.h"

// One driver engine for every external solver (plan 3.4 item 2). Owns the
// full run lifecycle previously duplicated across CBSDriver and
// LacamDriverBase: worker thread, cancellation (killing the child process),
// per-run map/scen generation, temp-file registration, argv execution via
// ProcessRunner, error reporting, and parsing through an ISolutionParser.
//
// Solver families differ only in (a) how the command line is built and
// which output/paths files it uses -- the InvocationBuilder -- and (b) how
// output is parsed -- the ISolutionParser. The public CBSDriver /
// LacamDriverBase classes are thin adapters over this engine.
class ExternalSolverDriver
{
public:
    // Command line plus the files the parser will read/produce.
    struct Invocation
    {
        std::vector<std::string> argv;
        std::string outputFile; // solver's own output (-o)
        std::string pathsFile;  // standard arrow-paths destination
    };

    // Builds the invocation from the generated instance files.
    // prefixPath is "<tempDir>/<mapName>_<timestamp>" (no extension).
    using InvocationBuilder =
        std::function<Invocation(const std::string& mapFile, const std::string& scenFile,
                                 const std::string& prefixPath, std::size_t agentCount)>;

    using OutcomeCallback = std::function<void(const SolverOutcome&)>;
    using TempFileCallback = std::function<void(const std::string&)>;

    // displayName: used in error messages ("CBS solver", "LaCAM2").
    // manifestKey: the solvers.yaml key quoted in configuration hints.
    // exampleExecutable: e.g. "/path/to/cbs", quoted in configuration hints.
    ExternalSolverDriver(std::string displayName, std::string manifestKey,
                         std::string exampleExecutable);
    ~ExternalSolverDriver();

    ExternalSolverDriver(const ExternalSolverDriver&) = delete;
    ExternalSolverDriver& operator=(const ExternalSolverDriver&) = delete;

    void setExecutablePath(const std::string& path) { m_executablePath = path; }
    const std::string& getExecutablePath() const { return m_executablePath; }

    bool isRunning() const { return m_isRunning.load(); }

    // Process-wide switch for the per-run diagnostic lines on stdout (the
    // full solver command, parser summaries). On by default; the batch
    // runner turns it off unless 'openmapf run --verbose' is given.
    static void setCommandLogging(bool enabled) { s_commandLogging.store(enabled); }
    static bool commandLogging() { return s_commandLogging.load(); }

    // Sets the cancel flag and kills the child process, if any. A cancelled
    // run does not invoke the outcome callback.
    void cancel();

    // Runs the solver on a background thread. No-op if already running.
    // The callback fires on the worker thread; the parser must outlive the
    // run (pass by shared_ptr).
    void solveAsync(const std::vector<Agent>& agents, const Environment& env,
                    const std::string& mapName, InvocationBuilder buildInvocation,
                    std::shared_ptr<ISolutionParser> parser, OutcomeCallback callback,
                    const std::string& tempDir = "", TempFileCallback tempFileCallback = nullptr);

private:
    SolverOutcome execute(const std::vector<Agent>& agents, const Environment& env,
                          const std::string& mapName, const InvocationBuilder& buildInvocation,
                          ISolutionParser& parser, const std::string& tempDir,
                          const TempFileCallback& tempFileCallback);

    std::string m_displayName;
    std::string m_manifestKey;
    std::string m_exampleExecutable;
    std::string m_executablePath; // empty = not configured

    ProcessRunner m_runner;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldCancel{false};
    std::thread m_solverThread;

    static inline std::atomic<bool> s_commandLogging{true};
};
