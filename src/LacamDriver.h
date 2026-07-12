#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "Environment.h"
#include "ExternalSolverDriver.h"
#include "Types.h"

// LaCAM Parameter Structure
struct LacamParameters
{
    // Core parameters
    int timeLimit = 10;    // -t parameter
    int seed = 0;          // -s parameter
    bool logShort = false; // -l parameter
    int verbose = 0;       // -v parameter

    // Grid configuration
    bool hexGrid = false;              // --grid_type hex
    std::string hexConfig = "odd-row"; // --hex_config
};

// LaCAM Results Structure
struct LacamResults
{
    bool solutionFound = false;
    int solutionCost = -1; // sum of costs (soc)
    int makespan = -1;
    double runtime = 0.0;
    int numAgents = 0;
    std::string errorMessage = "";
    std::string pathsFile = ""; // Path to the generated paths file

    // LaCAM specific metrics
    int socLowerBound = -1;
    int makespanLowerBound = -1;
    int sumOfLoss = -1;
    int sumOfLossLowerBound = -1;
};

// Base LaCAM driver: a thin adapter over the shared ExternalSolverDriver
// engine (plan 3.4 item 2). Owns only the LaCAM-specific parts: the command
// line (InvocationBuilder) and the output parsing (LacamOutputParser).
class LacamDriverBase
{
public:
    LacamDriverBase(const std::string& executablePath, const std::string& algorithmName);
    virtual ~LacamDriverBase() = default;

    // Main solve method
    void solveAsync(const std::vector<Agent>& agents, const Environment& env,
                    const LacamParameters& params, const std::string& mapName,
                    std::function<void(const LacamResults&)> callback,
                    const std::string& tempDir = "",
                    std::function<void(const std::string&)> tempFileCallback = nullptr);

    // Solver executable path ("" = not configured)
    void setExecutablePath(const std::string& path) { m_engine.setExecutablePath(path); }
    const std::string& getExecutablePath() const { return m_engine.getExecutablePath(); }

    // Status checking
    bool isRunning() const { return m_engine.isRunning(); }
    void cancel() { m_engine.cancel(); }

    // Results access (last completed run)
    const LacamResults& getResults() const { return m_results; }

    // Algorithm name
    const std::string& getAlgorithmName() const { return m_algorithmName; }

    // Parse a LaCAM output log ('key=value' lines). Returns the solved
    // flag; metric fields are filled in as encountered. Static and
    // stream-based so it is unit-testable against fixtures.
    static bool parseLacamOutput(std::istream& in, LacamResults& results);

    // Convert the 'solution=' section of a LaCAM log (per-timestep agent
    // positions) into the tool's per-agent arrow path format. Returns
    // false when no positions could be extracted.
    static bool convertLacamPaths(std::istream& in, std::ostream& out);

private:
    static std::string hexConfigString(const Environment& env);
    static std::string toLower(const std::string& s);

    std::string m_algorithmName;
    ExternalSolverDriver m_engine;
    LacamResults m_results;
};

// Specific driver classes for each LaCAM variant. Executable paths are not
// hardcoded: they come from the solver manifest via setExecutablePath();
// an empty path means "not configured".
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
