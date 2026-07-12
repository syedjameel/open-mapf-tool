#include "LacamDriver.h"
#include "MapFormat.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <regex>

LacamDriverBase::LacamDriverBase(const std::string& executablePath, const std::string& algorithmName)
    : m_executablePath(executablePath), m_algorithmName(algorithmName) {}

LacamDriverBase::~LacamDriverBase() {
    cancel();
    if (m_solverThread.joinable()) {
        m_solverThread.join();
    }
}

void LacamDriverBase::solveAsync(const std::vector<Agent>& agents,
                                 const Environment& env,
                                 const LacamParameters& params,
                                 const std::string& mapName,
                                 std::function<void(const LacamResults&)> callback,
                                 const std::string& tempDir,
                                 std::function<void(const std::string&)> tempFileCallback) {
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

    // Store temp file management info
    m_tempDir = tempDir;
    m_tempFileCallback = tempFileCallback;

    // Launch solver in background thread
    m_solverThread = std::thread([this, agents, env, params, mapName, callback]() {
        try {
            // Small delay to ensure thread setup
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            executeLacam(agents, env, params, mapName, m_tempDir, m_tempFileCallback);

            if (callback && !m_shouldCancel.load()) {
                callback(m_results);
            }
        } catch (const std::exception& e) {
            m_results.errorMessage = e.what();
            m_results.solutionFound = false;
            if (callback) {
                callback(m_results);
            }
        }

        m_isRunning.store(false);
    });
}

void LacamDriverBase::cancel() {
    m_shouldCancel.store(true);
}

void LacamDriverBase::generateMapFile(const Environment& env, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create map file: " + filename);
    }

    MapFormat::writeMap(file, env.m_grid, env.m_cols, env.m_rows);
    file.close();
}

void LacamDriverBase::generateScenarioFile(const std::vector<Agent>& agents, const Environment& env,
                                           const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create scenario file: " + filename);
    }

    // Extract map filename from scenario filename
    std::string mapFilename = filename.substr(0, filename.find_last_of('.')) + ".map";
    size_t lastSlash = mapFilename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        mapFilename = mapFilename.substr(lastSlash + 1);
    }

    MapFormat::writeScen(file, agents, mapFilename, env.m_cols, env.m_rows);
    file.close();
}

std::string LacamDriverBase::getHexConfigString(const Environment& env) const {
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

void LacamDriverBase::executeLacam(const std::vector<Agent>& agents,
                                   const Environment& env,
                                   const LacamParameters& params,
                                   const std::string& mapName,
                                   const std::string& tempDir,
                                   std::function<void(const std::string&)> tempFileCallback) {
    // Store current map name
    m_currentMapName = mapName;

    // Reset results
    m_results = LacamResults();

    // Count agents with goals
    size_t agentCount = 0;
    for (const auto& agent : agents) {
        if (agent.hasGoal) agentCount++;
    }

    if (agentCount == 0) {
        m_results.errorMessage = "No agents with goals found";
        return;
    }

    try {
        // Generate unique filenames with timestamp
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::string uniquePrefix = mapName + "_" + std::to_string(timestamp);

        std::string mapFile, scenFile, outputFile, pathsFile;

        if (!tempDir.empty()) {
            // Use temp directory
            mapFile = tempDir + "/" + uniquePrefix + ".map";
            scenFile = tempDir + "/" + uniquePrefix + ".scen";
            outputFile = tempDir + "/" + uniquePrefix + "_lacam.txt";
            pathsFile = tempDir + "/" + uniquePrefix + "_paths.txt";
        } else {
            // Use current directory
            mapFile = uniquePrefix + ".map";
            scenFile = uniquePrefix + ".scen";
            outputFile = uniquePrefix + "_lacam.txt";
            pathsFile = uniquePrefix + "_paths.txt";
        }

        generateMapFile(env, mapFile);
        generateScenarioFile(agents, env, scenFile);

        // Register temp files for cleanup if callback provided
        if (tempFileCallback) {
            tempFileCallback(mapFile);
            tempFileCallback(scenFile);
            tempFileCallback(outputFile);
            tempFileCallback(pathsFile);
        }

        if (m_shouldCancel.load()) return;

        // Build LaCAM command
        std::ostringstream cmd;
        cmd << m_executablePath;
        cmd << " -m " << mapFile;
        cmd << " -i " << scenFile;
        cmd << " -N " << agentCount;
        cmd << " -t " << params.timeLimit;
        cmd << " -s " << params.seed;
        cmd << " -v " << params.verbose;
        cmd << " -o " << outputFile;

        if (params.logShort) {
            cmd << " -l";
        }

        // Grid type configuration
        if (env.m_hexagon) {
            cmd << " -g hex";
            cmd << " -hc " << getHexConfigString(env);
        } else {
            cmd << " -g square";
        }

        // Algorithm-specific parameters for LaCAM2
        if (m_algorithmName == "LaCAM2") {
            // LaCAM2 has additional parameters like objective and restart_rate
            // These could be added to LacamParameters if needed
            cmd << " --objective 0";  // Default: no optimization
            cmd << " --restart_rate 0.001";  // Default restart rate
        }

        // Algorithm-specific parameters for LaCAM3
        if (m_algorithmName == "LaCAM3") {
            // LaCAM3 has many more parameters, using defaults for now
            // These could be exposed through LacamParameters if needed
        }

        // Redirect stderr to /dev/null to suppress verbose output
        cmd << " 2>/dev/null";

        std::cout << "[" << m_algorithmName << "] Executing command: " << cmd.str() << std::endl;

        // Execute LaCAM
        auto startTime = std::chrono::steady_clock::now();
        int result = std::system(cmd.str().c_str());
        auto endTime = std::chrono::steady_clock::now();

        m_results.runtime = std::chrono::duration<double>(endTime - startTime).count();

        if (m_shouldCancel.load()) return;

        if (result != 0) {
            m_results.errorMessage = m_algorithmName + " execution failed with code " + std::to_string(result);
            return;
        }

        // Parse the LaCAM output
        if (parseResults(outputFile)) {
            // Convert LaCAM paths to standard format
            if (convertLacamPathsToStandard(outputFile, pathsFile)) {
                m_results.pathsFile = pathsFile;
                m_results.solutionFound = true;
                m_results.numAgents = agentCount;
            } else {
                m_results.errorMessage = "Failed to convert " + m_algorithmName + " paths to standard format";
            }
        } else {
            m_results.errorMessage = "Failed to parse " + m_algorithmName + " results";
        }

    } catch (const std::exception& e) {
        m_results.errorMessage = m_algorithmName + " error: " + std::string(e.what());
    }
}

bool LacamDriverBase::parseResults(const std::string& outputFile) {
    std::ifstream file(outputFile);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        try {
            if (line.find("solved=") == 0 && line.length() > 7) {
                m_results.solutionFound = (line.substr(7) == "1");
            } else if (line.find("soc=") == 0 && line.length() > 4) {
                m_results.solutionCost = std::stoi(line.substr(4));
            } else if (line.find("soc_lb=") == 0 && line.length() > 7) {
                m_results.socLowerBound = std::stoi(line.substr(7));
            } else if (line.find("makespan=") == 0 && line.length() > 9) {
                m_results.makespan = std::stoi(line.substr(9));
            } else if (line.find("makespan_lb=") == 0 && line.length() > 12) {
                m_results.makespanLowerBound = std::stoi(line.substr(12));
            } else if (line.find("sum_of_loss=") == 0 && line.length() > 12) {
                m_results.sumOfLoss = std::stoi(line.substr(12));
            } else if (line.find("sum_of_loss_lb=") == 0 && line.length() > 15) {
                m_results.sumOfLossLowerBound = std::stoi(line.substr(15));
            } else if (line.find("comp_time=") == 0 && line.length() > 10) {
                // LaCAM reports time in milliseconds
                m_results.runtime = std::stod(line.substr(10)) / 1000.0;
            }
        } catch (const std::exception& e) {
            std::cerr << "[LaCAM] Error parsing line '" << line << "': " << e.what() << std::endl;
        }
    }

    file.close();
    return m_results.solutionFound;
}

bool LacamDriverBase::convertLacamPathsToStandard(const std::string& lacamOutputFile,
                                                  const std::string& standardPathFile) {
    std::ifstream inFile(lacamOutputFile);
    if (!inFile.is_open()) {
        return false;
    }

    std::vector<std::vector<std::pair<int, int>>> agentPaths;
    std::string line;
    bool inSolution = false;
    int maxTimeStep = 0;
    int mapHeight = -1;

    // First pass: get map height for coordinate conversion
    inFile.seekg(0);
    while (std::getline(inFile, line)) {
        if (line.find("map_file=") == 0) {
            // Extract map filename and read its height
            std::string mapFile = line.substr(9);
            // Try to find the map file in temp directory
            std::string mapPath = "temp/" + mapFile;
            std::ifstream mapIn(mapPath);
            if (mapIn.is_open()) {
                std::string mapLine;
                while (std::getline(mapIn, mapLine)) {
                    if (mapLine.find("height ") == 0) {
                        mapHeight = std::stoi(mapLine.substr(7));
                        break;
                    }
                }
                mapIn.close();
            }
            break;
        }
    }

    // Reset to beginning for second pass
    inFile.clear();
    inFile.seekg(0);

    // Parse the LaCAM output format
    while (std::getline(inFile, line)) {
        if (line == "solution=") {
            inSolution = true;
            continue;
        }

        if (!inSolution) continue;

        // Parse lines like "0:(1,0),(0,1),"
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

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
            while (agentPaths.size() <= agentIdx) {
                agentPaths.push_back(std::vector<std::pair<int, int>>());
            }

            // Ensure we have enough time steps
            while (agentPaths[agentIdx].size() <= timeStep) {
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

    inFile.close();

    if (agentPaths.empty()) {
        return false;
    }

    // Write in standard format
    std::ofstream outFile(standardPathFile);
    if (!outFile.is_open()) {
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
    std::cout << "[LaCAM] Converted " << agentPaths.size() << " agent paths, max timestep: " << maxTimeStep << std::endl;

    outFile.close();
    return true;
}