#include "CBSDriver.h"

#include <memory>
#include <sstream>

#include "SolutionParsers.h"

CBSDriver::CBSDriver() : m_engine("CBS solver", "cbs", "/path/to/cbs") {}

void CBSDriver::solveAsync(const std::vector<Agent>& agents,
                           const Environment& env,
                           const CBSParameters& params,
                           const std::string& mapName,
                           std::function<void(const CBSResults&)> callback,
                           const std::string& tempDir,
                           std::function<void(const std::string&)> tempFileCallback) {
    // CBS-specific command line; the engine owns everything else.
    auto buildInvocation = [params, exe = m_engine.getExecutablePath()](
                               const std::string& mapFile, const std::string& scenFile,
                               const std::string& prefixPath, std::size_t agentCount) {
        auto boolArg = [](bool b) { return std::string(b ? "true" : "false"); };
        ExternalSolverDriver::Invocation inv;
        inv.outputFile = prefixPath + ".csv";
        inv.pathsFile = prefixPath + ".txt";
        inv.argv = {
            exe,
            "-m", mapFile,
            "-a", scenFile,
            "-o", inv.outputFile,
            "-k", std::to_string(agentCount),
            "-t", std::to_string(params.cutoffTime),
            "--heuristics=" + params.heuristic,
            "--cluster_heuristics=" + params.clusterHeuristic,
            "--bypass=" + boolArg(params.bypass),
            "--rectangleReasoning=" + params.rectangleReasoning,
            "--corridorReasoning=" + params.corridorReasoning,
            "--targetReasoning=" + boolArg(params.targetReasoning),
            "--mutexReasoning=" + boolArg(params.mutexReasoning),
            "--prioritizingConflicts=" + boolArg(params.prioritizingConflicts),
            "--disjointSplitting=" + boolArg(params.disjointSplitting),
            "--sipp=" + boolArg(params.sipp),
            "--nodeLimit=" + std::to_string(params.nodeLimit),
            "--screen=" + std::to_string(params.screen),
            "--stats=" + boolArg(params.stats),
            "--restart=" + std::to_string(params.restart),
            "--seed=" + std::to_string(params.seed),
            "--hexGrid=" + boolArg(params.hexGrid),
            "--gridConfig=" + params.gridConfig,
            "--outputPaths=" + inv.pathsFile,
        };
        return inv;
    };

    auto outcomeToResults = [this, callback](const SolverOutcome& outcome) {
        CBSResults results;
        results.solutionFound = outcome.solved;
        results.runtime = outcome.runtimeS;
        results.solutionCost = static_cast<int>(outcome.cost);
        results.highLevelNodes = outcome.hlExpanded >= 0 ? static_cast<int>(outcome.hlExpanded) : 0;
        results.lowLevelNodes = outcome.llExpanded >= 0 ? static_cast<int>(outcome.llExpanded) : 0;
        results.errorMessage = outcome.errorMessage;
        results.pathsFile = outcome.pathsFile;

        m_results = results;
        if (callback) {
            callback(results);
        }
    };

    m_engine.solveAsync(agents, env, mapName, std::move(buildInvocation),
                        std::make_shared<Cbsh2CsvParser>(), std::move(outcomeToResults), tempDir,
                        std::move(tempFileCallback));
}

bool CBSDriver::parseResultsCsv(std::istream& in, CBSResults& results) {
    std::string line;
    std::getline(in, line); // Skip header

    std::string lastValidLine;
    // Read all lines and keep the last valid one
    while (std::getline(in, line)) {
        if (!line.empty() && line.find(',') != std::string::npos) {
            lastValidLine = line;
        }
    }

    if (!lastValidLine.empty()) {
        std::stringstream ss(lastValidLine);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        // CSV format: runtime,#high-level expanded,#high-level generated,#low-level expanded,#low-level generated,solution cost,...
        if (tokens.size() >= 6) {
            try {
                results.runtime = std::stod(tokens[0]);           // runtime
                results.highLevelNodes = std::stoi(tokens[1]);    // #high-level expanded
                results.lowLevelNodes = std::stoi(tokens[3]);     // #low-level expanded
                results.solutionCost = std::stoi(tokens[5]);      // solution cost

                results.solutionFound = (results.solutionCost != -1);

                return true;
            } catch (const std::exception&) {
                return false;
            }
        }
    }

    return false;
}
