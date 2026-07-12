#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include "Types.h"
#include "Environment.h"
#include "ProcessRunner.h"

// CBS Parameter Structure
struct CBSParameters
{
    // Core parameters
    int cutoffTime = 60;                     // -t parameter
    std::string heuristic = "WDG";           // --heuristic
    std::string clusterHeuristic = "N";      // --cluster_heuristics
    bool bypass = true;                      // --bypass
    std::string rectangleReasoning = "None"; // --rectangleReasoning
    std::string corridorReasoning = "None";  // --corridorReasoning
    bool targetReasoning = true;             // --targetReasoning
    bool mutexReasoning = false;             // --mutexReasoning
    bool hexGrid = false;                    // --hexGrid
    std::string gridConfig = "oddr";         // --gridConfig

    // Advanced parameters
    bool prioritizingConflicts = true;
    bool disjointSplitting = false;
    bool sipp = false;
    int nodeLimit = 1000000;
    int screen = 1;
    bool stats = false;
    int restart = 1;
    int seed = 0;
};

// CBS Results Structure
struct CBSResults
{
    bool solutionFound = false;
    int solutionCost = -1;
    double runtime = 0.0;
    int highLevelNodes = 0;
    int lowLevelNodes = 0;
    std::string errorMessage = "";
    std::string pathsFile = "";  // Path to the generated paths file
};

// CBS Driver Class
class CBSDriver
{
public:
    CBSDriver();
    ~CBSDriver();

    // Main solve method
    void solveAsync(const std::vector<Agent> &agents,
                    const Environment &env,
                    const CBSParameters &params,
                    const std::string &mapName,
                    std::function<void(const CBSResults &)> callback,
                    const std::string &tempDir = "",
                    std::function<void(const std::string &)> tempFileCallback = nullptr);

    // Solver executable path ("" = not configured)
    void setExecutablePath(const std::string &path) { m_executablePath = path; }
    const std::string &getExecutablePath() const { return m_executablePath; }

    // Status checking
    bool isRunning() const { return m_isRunning.load(); }
    void cancel();

    // Results access
    const CBSResults &getResults() const { return m_results; }

private:
    std::string m_executablePath; // empty = not configured
    ProcessRunner m_runner;
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldCancel{false};
    std::thread m_solverThread;
    CBSResults m_results;
    std::string m_currentMapName;
    std::string m_tempDir;
    std::function<void(const std::string &)> m_tempFileCallback;

    // File generation methods
    void generateMapFile(const Environment &env, const std::string &filename);
    void generateScenarioFile(const std::vector<Agent> &agents, const Environment &env,
                              const std::string &filename);

    // CBS execution
    void executeCBS(const std::vector<Agent> &agents,
                    const Environment &env,
                    const CBSParameters &params,
                    const std::string &mapName,
                    const std::string &tempDir,
                    std::function<void(const std::string &)> tempFileCallback);

    // Result parsing
    bool parseResults(const std::string &csvFile);

    // Utility methods
    std::string getGridConfigString(const Environment &env) const;
};