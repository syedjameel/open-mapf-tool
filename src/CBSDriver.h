#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "Environment.h"
#include "ExternalSolverDriver.h"
#include "Types.h"

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
    std::string pathsFile = ""; // Path to the generated paths file
};

// CBS driver: a thin adapter over the shared ExternalSolverDriver engine
//. Owns only the CBS-specific parts: the command line
// (InvocationBuilder) and the results-CSV parsing (Cbsh2CsvParser).
class CBSDriver
{
public:
    CBSDriver();
    ~CBSDriver() = default;

    // Main solve method
    void solveAsync(const std::vector<Agent>& agents, const Environment& env,
                    const CBSParameters& params, const std::string& mapName,
                    std::function<void(const CBSResults&)> callback,
                    const std::string& tempDir = "",
                    std::function<void(const std::string&)> tempFileCallback = nullptr);

    // Solver executable path ("" = not configured)
    void setExecutablePath(const std::string& path) { m_engine.setExecutablePath(path); }
    const std::string& getExecutablePath() const { return m_engine.getExecutablePath(); }

    // Status checking
    bool isRunning() const { return m_engine.isRunning(); }
    void cancel() { m_engine.cancel(); }

    // Results access (last completed run)
    const CBSResults& getResults() const { return m_results; }

    // Parse a CBSH2-RTC results CSV stream (header + one row per restart;
    // the last valid row wins). Returns false if no usable row exists.
    // Static and stream-based so it is unit-testable against fixtures.
    static bool parseResultsCsv(std::istream& in, CBSResults& results);

private:
    ExternalSolverDriver m_engine;
    CBSResults m_results;
};
