#include "ExternalSolverDriver.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "MapFormat.h"

namespace {

void writeMapFile(const Environment& env, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create map file: " + filename);
    }
    MapFormat::writeMap(file, env.m_grid, env.m_cols, env.m_rows);
}

void writeScenFile(const std::vector<Agent>& agents, const Environment& env,
                   const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create scenario file: " + filename);
    }

    // The scen references its sibling map file by basename
    std::string mapFilename = filename.substr(0, filename.find_last_of('.')) + ".map";
    size_t lastSlash = mapFilename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        mapFilename = mapFilename.substr(lastSlash + 1);
    }
    MapFormat::writeScen(file, agents, mapFilename, env.m_cols, env.m_rows);
}

} // namespace

ExternalSolverDriver::ExternalSolverDriver(std::string displayName, std::string manifestKey,
                                           std::string exampleExecutable)
    : m_displayName(std::move(displayName)), m_manifestKey(std::move(manifestKey)),
      m_exampleExecutable(std::move(exampleExecutable))
{
}

ExternalSolverDriver::~ExternalSolverDriver()
{
    cancel();
    if (m_solverThread.joinable()) {
        m_solverThread.join();
    }
}

void ExternalSolverDriver::cancel()
{
    m_shouldCancel.store(true);
    m_runner.terminate(); // actually stop the child process, if any
}

void ExternalSolverDriver::solveAsync(const std::vector<Agent>& agents, const Environment& env,
                                      const std::string& mapName,
                                      InvocationBuilder buildInvocation,
                                      std::shared_ptr<ISolutionParser> parser,
                                      OutcomeCallback callback, const std::string& tempDir,
                                      TempFileCallback tempFileCallback)
{
    if (m_isRunning.load()) {
        return; // Already running
    }

    // Clean up any previous thread
    if (m_solverThread.joinable()) {
        m_shouldCancel.store(true);
        m_solverThread.join();
    }

    m_shouldCancel.store(false);
    m_isRunning.store(true);

    m_solverThread = std::thread([this, agents, env, mapName,
                                  buildInvocation = std::move(buildInvocation),
                                  parser = std::move(parser), callback = std::move(callback),
                                  tempDir, tempFileCallback = std::move(tempFileCallback)]() {
        try {
            SolverOutcome outcome =
                execute(agents, env, mapName, buildInvocation, *parser, tempDir,
                        tempFileCallback);

            if (callback && !m_shouldCancel.load()) {
                callback(outcome);
            }
        } catch (const std::exception& e) {
            SolverOutcome outcome;
            outcome.errorMessage = m_displayName + " error: " + std::string(e.what());
            if (callback) {
                callback(outcome);
            }
        }

        m_isRunning.store(false);
    });
}

SolverOutcome ExternalSolverDriver::execute(const std::vector<Agent>& agents,
                                            const Environment& env, const std::string& mapName,
                                            const InvocationBuilder& buildInvocation,
                                            ISolutionParser& parser, const std::string& tempDir,
                                            const TempFileCallback& tempFileCallback)
{
    SolverOutcome outcome;

    // Count agents with goals
    std::size_t agentCount = 0;
    for (const auto& agent : agents) {
        if (agent.hasGoal) {
            agentCount++;
        }
    }
    if (agentCount == 0) {
        outcome.errorMessage = "No agents with goals found";
        return outcome;
    }
    outcome.agents = static_cast<long>(agentCount);

    // Degrade gracefully when the solver binary is absent: report a clear,
    // actionable error instead of executing a nonexistent path.
    if (m_executablePath.empty()) {
        outcome.errorMessage = m_displayName + " is not configured. Set 'solvers: " +
                               m_manifestKey + ": " + m_exampleExecutable +
                               "' in your env.yaml (run 'openmapf --help' for config locations).";
        return outcome;
    }
    if (!std::filesystem::exists(m_executablePath)) {
        outcome.errorMessage = m_displayName + " executable not found: " + m_executablePath +
                               " (check the 'solvers: " + m_manifestKey +
                               ":' entry in your env.yaml).";
        return outcome;
    }

    // Unique per-run file names
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
    const std::string uniquePrefix = mapName + "_" + std::to_string(timestamp);
    const std::string prefixPath =
        tempDir.empty() ? uniquePrefix : tempDir + "/" + uniquePrefix;

    const std::string mapFile = prefixPath + ".map";
    const std::string scenFile = prefixPath + ".scen";

    writeMapFile(env, mapFile);
    writeScenFile(agents, env, scenFile);

    Invocation inv = buildInvocation(mapFile, scenFile, prefixPath, agentCount);

    // Register temp files for cleanup if callback provided
    if (tempFileCallback) {
        tempFileCallback(mapFile);
        tempFileCallback(scenFile);
        tempFileCallback(inv.outputFile);
        tempFileCallback(inv.pathsFile);
    }

    if (m_shouldCancel.load()) {
        return outcome;
    }

    std::cout << "[" << m_displayName << "] Executing: " << ProcessRunner::joinArgv(inv.argv)
              << std::endl;

    const auto startTime = std::chrono::steady_clock::now();
    ProcessRunner::Result run = m_runner.run(inv.argv);
    const auto endTime = std::chrono::steady_clock::now();
    outcome.runtimeS = std::chrono::duration<double>(endTime - startTime).count();

    if (m_shouldCancel.load() || run.terminated) {
        return outcome;
    }

    if (!run.started) {
        outcome.errorMessage = "Failed to launch " + m_displayName + ": " + run.errorMessage;
        return outcome;
    }
    if (run.exitCode != 0) {
        outcome.errorMessage =
            m_displayName + " exited with code " + std::to_string(run.exitCode);
        if (!run.stderrText.empty()) {
            outcome.errorMessage += "\nstderr: " + ProcessRunner::excerpt(run.stderrText);
        }
        return outcome;
    }

    parser.parse(inv.outputFile, inv.pathsFile, outcome);
    return outcome;
}
