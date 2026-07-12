#include "LacamDriver.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>
#include <vector>

#include "SolutionParsers.h"

std::string LacamDriverBase::toLower(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

LacamDriverBase::LacamDriverBase(const std::string& executablePath,
                                 const std::string& algorithmName)
    : m_algorithmName(algorithmName),
      m_engine(algorithmName, toLower(algorithmName), "/path/to/main")
{
    m_engine.setExecutablePath(executablePath);
}

std::string LacamDriverBase::hexConfigString(const Environment& env)
{
    switch (env.m_hex_grid_type) {
    case HexGridType::ODD_R:
        return "odd-row";
    case HexGridType::EVEN_R:
        return "even-row";
    case HexGridType::ODD_Q:
        return "odd-column";
    case HexGridType::EVEN_Q:
        return "even-column";
    default:
        return "odd-row";
    }
}

void LacamDriverBase::solveAsync(const std::vector<Agent>& agents, const Environment& env,
                                 const LacamParameters& params, const std::string& mapName,
                                 std::function<void(const LacamResults&)> callback,
                                 const std::string& tempDir,
                                 std::function<void(const std::string&)> tempFileCallback)
{
    // LaCAM-specific command line; the engine owns everything else.
    const bool hex = env.m_hexagon;
    const std::string hexConfig = hexConfigString(env);
    auto buildInvocation = [params, hex, hexConfig, name = m_algorithmName,
                            exe = m_engine.getExecutablePath()](
                               const std::string& mapFile, const std::string& scenFile,
                               const std::string& prefixPath, std::size_t agentCount) {
        ExternalSolverDriver::Invocation inv;
        inv.outputFile = prefixPath + "_lacam.txt";
        inv.pathsFile = prefixPath + "_paths.txt";
        inv.argv = {
            exe,
            "-m",
            mapFile,
            "-i",
            scenFile,
            "-N",
            std::to_string(agentCount),
            "-t",
            std::to_string(params.timeLimit),
            "-s",
            std::to_string(params.seed),
            "-v",
            std::to_string(params.verbose),
            "-o",
            inv.outputFile,
        };

        if (params.logShort) {
            inv.argv.push_back("-l");
        }

        // Grid type configuration
        if (hex) {
            inv.argv.push_back("-g");
            inv.argv.push_back("hex");
            inv.argv.push_back("-hc");
            inv.argv.push_back(hexConfig);
        } else {
            inv.argv.push_back("-g");
            inv.argv.push_back("square");
        }

        // Algorithm-specific parameters for LaCAM2
        if (name == "LaCAM2") {
            inv.argv.push_back("--objective");
            inv.argv.push_back("0"); // default: no optimization
            inv.argv.push_back("--restart_rate");
            inv.argv.push_back("0.001"); // default restart rate
        }
        // (LaCAM3 uses its defaults; expose via LacamParameters if needed)

        return inv;
    };

    auto outcomeToResults = [this, callback](const SolverOutcome& outcome) {
        LacamResults results;
        results.solutionFound = outcome.solved;
        results.runtime = outcome.runtimeS;
        results.solutionCost = static_cast<int>(outcome.cost);
        results.makespan = static_cast<int>(outcome.makespan);
        results.numAgents = static_cast<int>(outcome.agents);
        results.socLowerBound = static_cast<int>(outcome.socLowerBound);
        results.makespanLowerBound = static_cast<int>(outcome.makespanLowerBound);
        results.sumOfLoss = static_cast<int>(outcome.sumOfLoss);
        results.sumOfLossLowerBound = static_cast<int>(outcome.sumOfLossLowerBound);
        results.errorMessage = outcome.errorMessage;
        results.pathsFile = outcome.pathsFile;

        m_results = results;
        if (callback) {
            callback(results);
        }
    };

    m_engine.solveAsync(agents, env, mapName, std::move(buildInvocation),
                        std::make_shared<LacamOutputParser>(m_algorithmName),
                        std::move(outcomeToResults), tempDir, std::move(tempFileCallback));
}

bool LacamDriverBase::parseLacamOutput(std::istream& in, LacamResults& results)
{
    std::string line;
    while (std::getline(in, line)) {
        try {
            if (line.find("solved=") == 0 && line.length() > 7) {
                results.solutionFound = (line.substr(7) == "1");
            } else if (line.find("soc=") == 0 && line.length() > 4) {
                results.solutionCost = std::stoi(line.substr(4));
            } else if (line.find("soc_lb=") == 0 && line.length() > 7) {
                results.socLowerBound = std::stoi(line.substr(7));
            } else if (line.find("makespan=") == 0 && line.length() > 9) {
                results.makespan = std::stoi(line.substr(9));
            } else if (line.find("makespan_lb=") == 0 && line.length() > 12) {
                results.makespanLowerBound = std::stoi(line.substr(12));
            } else if (line.find("sum_of_loss=") == 0 && line.length() > 12) {
                results.sumOfLoss = std::stoi(line.substr(12));
            } else if (line.find("sum_of_loss_lb=") == 0 && line.length() > 15) {
                results.sumOfLossLowerBound = std::stoi(line.substr(15));
            } else if (line.find("comp_time=") == 0 && line.length() > 10) {
                // LaCAM reports time in milliseconds
                results.runtime = std::stod(line.substr(10)) / 1000.0;
            }
        } catch (const std::exception& e) {
            std::cerr << "[LaCAM] Error parsing line '" << line << "': " << e.what() << std::endl;
        }
    }

    return results.solutionFound;
}

bool LacamDriverBase::convertLacamPaths(std::istream& inFile, std::ostream& outFile)
{
    std::vector<std::vector<std::pair<int, int>>> agentPaths;
    std::string line;
    bool inSolution = false;
    int maxTimeStep = 0;

    // Parse the LaCAM output format
    while (std::getline(inFile, line)) {
        if (line == "solution=") {
            inSolution = true;
            continue;
        }

        if (!inSolution)
            continue;

        // Parse lines like "0:(1,0),(0,1),"
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        int timeStep = std::stoi(line.substr(0, colonPos));
        maxTimeStep = std::max(maxTimeStep, timeStep);

        // Parse the positions
        std::string positions = line.substr(colonPos + 1);
        std::regex posRegex(R"(\((\d+),(\d+)\))");
        std::sregex_iterator iter(positions.begin(), positions.end(), posRegex);
        std::sregex_iterator end;

        int agentIdx = 0;
        for (; iter != end; ++iter) {
            std::smatch match = *iter;
            int x = std::stoi(match[1]);
            int y = std::stoi(match[2]);

            // LaCAM uses (x,y) with origin at top-left, y increases downward
            // CBS/visualization expects (col, row) format
            // No coordinate transformation needed - LaCAM already provides correct format

            // Ensure we have enough agents
            while (agentPaths.size() <= static_cast<size_t>(agentIdx)) {
                agentPaths.push_back(std::vector<std::pair<int, int>>());
            }

            // Ensure we have enough time steps
            while (agentPaths[agentIdx].size() <= static_cast<size_t>(timeStep)) {
                if (agentPaths[agentIdx].empty()) {
                    agentPaths[agentIdx].push_back({x, y});
                } else {
                    // Fill with last position if needed
                    agentPaths[agentIdx].push_back(agentPaths[agentIdx].back());
                }
            }

            agentPaths[agentIdx][timeStep] = {x, y};
            agentIdx++;
        }
    }

    if (agentPaths.empty()) {
        return false;
    }

    // Write paths for each agent in the format expected by PathManager
    for (size_t i = 0; i < agentPaths.size(); ++i) {
        outFile << "Agent " << i << ": ";

        bool first = true;
        for (const auto& pos : agentPaths[i]) {
            if (!first) {
                outFile << "->";
            }
            outFile << "(" << pos.first << "," << pos.second << ")";
            first = false;
        }

        // Pad with final position if needed
        if (!agentPaths[i].empty()) {
            auto lastPos = agentPaths[i].back();
            for (int t = agentPaths[i].size(); t <= maxTimeStep; ++t) {
                outFile << "->(" << lastPos.first << "," << lastPos.second << ")";
            }
        }

        outFile << "\n";
    }

    // Debug output
    std::cout << "[LaCAM] Converted " << agentPaths.size()
              << " agent paths, max timestep: " << maxTimeStep << std::endl;

    return true;
}
