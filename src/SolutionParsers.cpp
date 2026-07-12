#include "SolutionParsers.h"

#include <fstream>

#include "CBSDriver.h"
#include "LacamDriver.h"

void Cbsh2CsvParser::parse(const std::string& outputFile, const std::string& pathsFile,
                           SolverOutcome& outcome)
{
    std::ifstream file(outputFile);
    CBSResults results;
    if (!file.is_open() || !CBSDriver::parseResultsCsv(file, results)) {
        outcome.errorMessage = "Failed to parse CBS results";
        return;
    }

    outcome.solved = results.solutionFound;
    outcome.runtimeS = results.runtime;
    outcome.cost = results.solutionCost;
    outcome.hlExpanded = results.highLevelNodes;
    outcome.llExpanded = results.lowLevelNodes;
    if (outcome.solved) {
        // CBS writes the arrow paths file itself via --outputPaths
        outcome.pathsFile = pathsFile;
    }
}

void LacamOutputParser::parse(const std::string& outputFile, const std::string& pathsFile,
                              SolverOutcome& outcome)
{
    std::ifstream file(outputFile);
    if (!file.is_open()) {
        outcome.errorMessage = "Failed to parse " + m_algorithmName + " results";
        return;
    }

    LacamResults results;
    const bool solved = LacamDriverBase::parseLacamOutput(file, results);

    // Copy whatever metrics the log reported, solved or not. comp_time
    // (when present) is more precise than the engine's wall clock.
    outcome.cost = results.solutionCost;
    outcome.makespan = results.makespan;
    outcome.socLowerBound = results.socLowerBound;
    outcome.makespanLowerBound = results.makespanLowerBound;
    outcome.sumOfLoss = results.sumOfLoss;
    outcome.sumOfLossLowerBound = results.sumOfLossLowerBound;
    if (results.runtime > 0.0) {
        outcome.runtimeS = results.runtime;
    }

    if (!solved) {
        outcome.errorMessage = "Failed to parse " + m_algorithmName + " results";
        return;
    }

    // Convert the per-timestep solution section into per-agent arrow paths
    std::ifstream in(outputFile);
    std::ofstream out(pathsFile);
    if (!in.is_open() || !out.is_open() || !LacamDriverBase::convertLacamPaths(in, out)) {
        outcome.errorMessage =
            "Failed to convert " + m_algorithmName + " paths to standard format";
        return;
    }

    outcome.solved = true;
    outcome.pathsFile = pathsFile;
}
