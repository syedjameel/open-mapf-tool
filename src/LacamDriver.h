#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include "Types.h"
#include "Environment.h"

// LaCAM Parameter Structure
struct LacamParameters
{
    // Core parameters
    int timeLimit = 10;                      // -t parameter
    int seed = 0;                            // -s parameter
    bool logShort = false;                   // -l parameter
    int verbose = 0;                         // -v parameter

    // Grid configuration
    bool hexGrid = false;                    // --grid_type hex
    std::string hexConfig = "odd-row";       // --hex_config
};

// LaCAM Results Structure
struct LacamResults
{
    bool solutionFound = false;
    int solutionCost = -1;  // sum of costs (soc)
    int makespan = -1;
    double runtime = 0.0;
    int numAgents = 0;
    std::string errorMessage = "";
    std::string pathsFile = "";  // Path to the generated paths file

    // LaCAM specific metrics
    int socLowerBound = -1;
    int makespanLowerBound = -1;
    int sumOfLoss = -1;
    int sumOfLossLowerBound = -1;
};

// Base LaCAM Driver Class
class LacamDriverBase
{
public:
    LacamDriverBase(const std::string& executablePath, const std::string& algorithmName);
    virtual ~LacamDriverBase();

    // Main solve method
    void solveAsync(const std::vector<Agent>& agents,
                    const Environment& env,
                    const LacamParameters& params,
                    const std::string& mapName,
                    std::function<void(const LacamResults&)> callback,
                    const std::string& tempDir = "",
                    std::function<void(const std::string&)> tempFileCallback = nullptr);

    // Solver executable path ("" = not configured)
    void setExecutablePath(const std::string& path) { m_executablePath = path; }
    const std::string& getExecutablePath() const { return m_executablePath; }

    // Status checking
    bool isRunning() const { return m_isRunning.load(); }
    void cancel();

    // Results access
    const LacamResults& getResults() const { return m_results; }

    // Algorithm name
    const std::string& getAlgorithmName() const { return m_algorithmName; }

protected:
    std::string m_executablePath;
    std::string m_algorithmName;

private:
    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldCancel{false};
    std::thread m_solverThread;
    LacamResults m_results;
    std::string m_currentMapName;
    std::string m_tempDir;
    std::function<void(const std::string&)> m_tempFileCallback;

    // File generation methods
    void generateMapFile(const Environment& env, const std::string& filename);
    void generateScenarioFile(const std::vector<Agent>& agents, const Environment& env,
                              const std::string& filename);

    // LaCAM execution
    void executeLacam(const std::vector<Agent>& agents,
                      const Environment& env,
                      const LacamParameters& params,
                      const std::string& mapName,
                      const std::string& tempDir,
                      std::function<void(const std::string&)> tempFileCallback);

    // Result parsing
    bool parseResults(const std::string& outputFile);
    bool convertLacamPathsToStandard(const std::string& lacamOutputFile,
                                     const std::string& standardPathFile);

    // Utility methods
    std::string getHexConfigString(const Environment& env) const;
};

// Specific driver classes for each LaCAM variant. Executable paths are not
// hardcoded: they come from the 'solvers' section of env.yaml via
// setExecutablePath(); an empty path means "not configured".
class LacamDriver : public LacamDriverBase
{
public:
    LacamDriver() : LacamDriverBase("", "LaCAM") {}
};

class Lacam2Driver : public LacamDriverBase
{
public:
    Lacam2Driver() : LacamDriverBase("", "LaCAM2") {}
};

class Lacam3Driver : public LacamDriverBase
{
public:
    Lacam3Driver() : LacamDriverBase("", "LaCAM3") {}
};