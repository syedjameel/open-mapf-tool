#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Batch experiment specification (experiment.yaml). Grammar documented in
// docs/experiments.md. The run matrix is
//   solvers x instances x grids x agents x seeds
// expanded in that (deterministic) order, then shuffled with a fixed,
// recorded seed so heavy instances spread across time (reference design:
// the authors' benchmark-v3 E1/E2 runners).
struct ExperimentInstance
{
    std::string mapPath;  // absolute (resolved against the spec file's dir)
    std::string scenPath; // absolute (resolved against the spec file's dir)
};

struct ExperimentSpec
{
    std::string name;
    std::string specPath;  // where the spec was loaded from (absolute)
    std::string outputDir; // results.csv + manifest.yaml live here (absolute)
    int timeLimitS = 30;
    std::uint32_t shuffleSeed = 0;
    std::vector<int> seeds;                    // default {0}
    std::vector<int> agentLadder;              // required
    std::vector<std::string> solvers;          // required (manifest keys)
    std::vector<std::string> grids;            // default {"square"}
    std::vector<ExperimentInstance> instances; // required

    // Parse + validate. Throws std::runtime_error with an actionable
    // message on any spec error (missing fields, bad grid tags, missing
    // map/scen files, non-positive values).
    static ExperimentSpec loadFromFile(const std::string& path);
};

// One cell of the expanded run matrix.
struct RunTask
{
    std::string solver;   // manifest key
    std::string mapPath;  // absolute
    std::string scenPath; // absolute
    std::string grid;     // "square" or "hex-oddr"/"hex-evenr"/"hex-oddq"/"hex-evenq"
    int agents = 0;
    int seed = 0;
    int timeLimitS = 0;

    std::string mapName() const;   // basename without extension
    std::string scenName() const;  // basename with extension
    std::string gridType() const;  // "square" or "hex"
    std::string hexLayout() const; // "", "oddr", ...

    // Resume key: matches the first 8 columns of the results CSV.
    std::string key() const;
    static std::string keyFromCsvRow(const std::vector<std::string>& fields);
};

// Expand the spec into the run matrix (deterministic order, pre-shuffle).
std::vector<RunTask> expandMatrix(const ExperimentSpec& spec);

// Deterministic Fisher-Yates shuffle (identical on every platform for the
// same seed; resume depends on this stability only for even load spreading,
// not correctness).
void shuffleTasks(std::vector<RunTask>& tasks, std::uint32_t seed);
