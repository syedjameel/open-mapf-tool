#include "CBSDriver.h"
#include "MapFormat.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <chrono>

CBSDriver::CBSDriver() = default;

CBSDriver::~CBSDriver() {
    cancel();
    if (m_solverThread.joinable()) {
        m_solverThread.join();
    }
}

void CBSDriver::solveAsync(const std::vector<Agent>& agents, 
                          const Environment& env,
                          const CBSParameters& params,
                          const std::string& mapName,
                          std::function<void(const CBSResults&)> callback,
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
    
    // Launch solver in background thread with high priority
    m_solverThread = std::thread([this, agents, env, params, mapName, callback]() {
        try {
            // Set high priority for solver thread
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            
            executeCBS(agents, env, params, mapName, m_tempDir, m_tempFileCallback);
            
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

void CBSDriver::cancel() {
    m_shouldCancel.store(true);
    m_runner.terminate(); // actually stop the child process, if any
}

void CBSDriver::generateMapFile(const Environment& env, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create map file: " + filename);
    }
    
    MapFormat::writeMap(file, env.m_grid, env.m_cols, env.m_rows);
    file.close();
}

void CBSDriver::generateScenarioFile(const std::vector<Agent>& agents, const Environment& env,
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

void CBSDriver::executeCBS(const std::vector<Agent>& agents,
                          const Environment& env,
                          const CBSParameters& params,
                          const std::string& mapName,
                          const std::string& tempDir,
                          std::function<void(const std::string&)> tempFileCallback) {
    // Store current map name 
    m_currentMapName = mapName;
    
    // Reset results
    m_results = CBSResults();
    
    // Count agents with goals
    size_t agentCount = 0;
    for (const auto& agent : agents) {
        if (agent.hasGoal) agentCount++;
    }
    
    if (agentCount == 0) {
        m_results.errorMessage = "No agents with goals found";
        return;
    }

    // Degrade gracefully when the solver binary is absent: report a clear,
    // actionable error instead of shelling out to a nonexistent path.
    if (m_executablePath.empty()) {
        m_results.errorMessage =
            "CBS solver is not configured. Set 'solvers: cbs: /path/to/cbs' "
            "in your env.yaml (run 'openmapf --help' for config locations).";
        return;
    }
    if (!std::filesystem::exists(m_executablePath)) {
        m_results.errorMessage =
            "CBS solver executable not found: " + m_executablePath +
            " (check the 'solvers: cbs:' entry in your env.yaml).";
        return;
    }

    try {
        // Generate unique file names to avoid conflicts
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        std::string uniquePrefix = mapName + "_" + std::to_string(timestamp);
        
        std::string mapFile, scenFile, outputFile, pathsFile;
        
        if (!tempDir.empty()) {
            // Use temp directory
            mapFile = tempDir + "/" + uniquePrefix + ".map";
            scenFile = tempDir + "/" + uniquePrefix + ".scen";
            outputFile = tempDir + "/" + uniquePrefix + ".csv";
            pathsFile = tempDir + "/" + uniquePrefix + ".txt";
        } else {
            // Use current directory (original behavior)
            mapFile = uniquePrefix + ".map";
            scenFile = uniquePrefix + ".scen";
            outputFile = uniquePrefix + ".csv";
            pathsFile = uniquePrefix + ".txt";
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
        
        // Build CBS command (argv form: executed directly, never via a shell)
        auto boolArg = [](bool b) { return std::string(b ? "true" : "false"); };
        std::vector<std::string> argv = {
            m_executablePath,
            "-m", mapFile,
            "-a", scenFile,
            "-o", outputFile,
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
            "--outputPaths=" + pathsFile,
        };

        if (m_shouldCancel.load()) return;

        // Execute CBS solver
        std::cout << "Executing CBS: " << ProcessRunner::joinArgv(argv) << std::endl;
        ProcessRunner::Result run = m_runner.run(argv);

        if (m_shouldCancel.load() || run.terminated) return;

        if (!run.started) {
            m_results.errorMessage = "Failed to launch CBS solver: " + run.errorMessage;
            return;
        }
        if (run.exitCode != 0) {
            m_results.errorMessage =
                "CBS solver exited with code " + std::to_string(run.exitCode);
            if (!run.stderrText.empty()) {
                m_results.errorMessage += "\nstderr: " + ProcessRunner::excerpt(run.stderrText);
            }
            return;
        }
        
        // Parse results
        if (!parseResults(outputFile)) {
            m_results.errorMessage = "Failed to parse CBS results";
        }
        
        // Store paths file location for external loading
        if (m_results.solutionFound) {
            m_results.pathsFile = pathsFile;
        }
        
        // Keep all files - never delete them
        
    } catch (const std::exception& e) {
        m_results.errorMessage = "CBS execution error: " + std::string(e.what());
    }
}

bool CBSDriver::parseResults(const std::string& csvFile) {
    std::ifstream file(csvFile);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::getline(file, line); // Skip header
    
    std::string lastValidLine;
    // Read all lines and keep the last valid one
    while (std::getline(file, line)) {
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
                m_results.runtime = std::stod(tokens[0]);           // runtime
                m_results.highLevelNodes = std::stoi(tokens[1]);    // #high-level expanded
                m_results.lowLevelNodes = std::stoi(tokens[3]);     // #low-level expanded  
                m_results.solutionCost = std::stoi(tokens[5]);      // solution cost
                
                m_results.solutionFound = (m_results.solutionCost != -1);
                
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }
    }
    
    return false;
}


std::string CBSDriver::getGridConfigString(const Environment& env) const {
    if (!env.m_hexagon) {
        return "square";
    }
    
    switch (env.m_hex_grid_type) {
        case HexGridType::ODD_R: return "oddr";
        case HexGridType::EVEN_R: return "evenr";
        case HexGridType::ODD_Q: return "oddq";
        case HexGridType::EVEN_Q: return "evenq";
        default: return "oddr";
    }
}

