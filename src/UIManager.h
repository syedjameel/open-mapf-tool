#pragma once

#include "imgui.h"
#include "imfilebrowser.h"
#include "Environment.h"
#include "PathManager.h"
#include "AnimationController.h"
#include "Types.h"
#include "CBSDriver.h"
#include "LacamDriver.h"
#include "ExperimentRunner.h"
#include "ExperimentSpec.h"
#include "MapFormat.h"
#include "ResultsCsv.h"
#include "SolverManifest.h"
#include <vector>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <chrono>
#include <filesystem>
#include <algorithm>

class UIManager {
public:
    UIManager(Environment& env, PathManager& pathManager, AnimationController& animController,
              SolverManifest& solverManifest);
    ~UIManager(); // Destructor for cleanup
    
    void setupStyle();
    void setupFileDialogs();
    
    void renderSceneWindow(ImDrawList* draw_list, const ImVec2& canvas_pos, const ImVec2& canvas_size, const ImVec2& origin, float zoom);
    void renderControlPanel();
    
    void handleFileDialogs();

    // Must be called once per frame on the GUI thread: consumes solver
    // results queued by the solver threads and applies them to GUI-owned
    // state (path manager, animation controller, dialogs).
    void processPendingResults();

    bool isAddRemoveObstacles() const { return m_addRemoveObstacles; }
    void setAddRemoveObstacles(bool value) { m_addRemoveObstacles = value; }
    
    bool isAddRemoveAgents() const { return m_addRemoveAgents; }
    void setAddRemoveAgents(bool value) { m_addRemoveAgents = value; }
    
    bool isLacamFormat() const { return m_lacamFormat; }
    
    // Agent interaction methods
    void handleAgentInteraction(const ImVec2& mousePos, const ImVec2& origin, float zoom, bool isHovered, bool isLeftClicked);
    const std::vector<Agent>& getDynamicAgents() const { return m_dynamicAgents; }
    std::string getCurrentMapName() const { return m_currentMapName; }
    
private:
    // UI state
    bool m_addRemoveObstacles = false;
    bool m_addRemoveAgents = false;
    bool m_lacamFormat = false;
    
    // Dynamic agent management
    std::vector<Agent> m_dynamicAgents;
    size_t m_nextAgentId = 0;
    size_t m_pendingAgentIndex = SIZE_MAX;  // Index of agent awaiting goal placement
    std::string m_currentMapName = "";
    
    // UI dialogs
    bool m_showMapNameDialog = false;
    char m_mapNameBuffer[256] = "";
    bool m_showSaveScenarioDialog = false;
    bool m_showProgressDialog = false;
    bool m_showParametersDialog = false;
    bool m_showResultsPanel = false;
    
    
    // File dialogs
    ImGui::FileBrowser m_fileDialog;
    ImGui::FileBrowser m_pathsFileDialog;
    ImGui::FileBrowser m_scenFileDialog;
    ImGui::FileBrowser m_solverExeDialog;
    int m_solverExeTarget = -1;    // manifest entry being browsed for
    std::string m_solverSaveStatus;

    // Scenario loading (.scen)
    bool m_showScenLoadDialog = false;
    MapFormat::ScenFile m_pendingScen;
    std::string m_pendingScenPath;
    int m_scenAgentCount = 10;
    std::string m_scenLoadStatus;
    std::string m_currentScenarioName = "interactive";

    // Results CSV export
    char m_resultsCsvPathBuffer[512] = "openmapf-results.csv";
    std::string m_resultsExportStatus;

    // Experiments tab (thin GUI over the headless runner; the CLI
    // 'openmapf run' is the primary interface)
    ImGui::FileBrowser m_experimentFileDialog;
    std::optional<ExperimentSpec> m_experimentSpec;
    std::string m_experimentStatus;
    bool m_experimentResume = false;
    std::unique_ptr<ExperimentRunner> m_experimentRunner;
    std::thread m_experimentThread;
    std::mutex m_experimentProgressMutex;
    RunnerProgress m_experimentProgress;
    std::atomic<bool> m_experimentRunning{false};
    int m_experimentExitCode = -1;
    
    // References to other components
    Environment& m_env;
    PathManager& m_pathManager;
    AnimationController& m_animController;
    SolverManifest& m_solvers;
    
    // Algorithm selection
    std::vector<std::string> m_algoNames = {"CBS", "LaCAM", "LaCAM2", "LaCAM3"};
    int m_selectedAlgo = 0;
    bool m_showAlgoParams = false;

    // CBS integration
    CBSDriver m_cbsDriver;
    CBSParameters m_cbsParams;
    CBSResults m_cbsResults;
    bool m_cbsRunning = false;
    bool m_startCBSAfterSave = false;
    std::chrono::steady_clock::time_point m_cbsStartTime;

    // Solver results are produced on solver threads. They are only queued
    // here (under the mutex) and consumed on the GUI thread by
    // processPendingResults(), so GUI-owned state is never mutated
    // concurrently with rendering.
    std::mutex m_pendingResultsMutex;
    std::optional<CBSResults> m_pendingCbsResults;
    std::optional<LacamResults> m_pendingLacamResults;

    // LaCAM integration
    LacamDriver m_lacamDriver;
    Lacam2Driver m_lacam2Driver;
    Lacam3Driver m_lacam3Driver;
    LacamParameters m_lacamParams;
    LacamResults m_lacamResults;
    bool m_lacamRunning = false;
    bool m_startLacamAfterSave = false;
    std::chrono::steady_clock::time_point m_lacamStartTime;
    
    // Recent files
    std::vector<std::string> m_recentFiles;
    std::string m_currentMapPath;
    std::string m_currentPathFile;
    
    // Temporary file management
    bool m_useTemporaryFiles = false;
    std::vector<std::string> m_tempFiles;
    std::string m_tempDirPath;
    
    // UI rendering methods
    void renderMapTab();
    void renderSolversTab();
    void renderExperimentsTab();
    void renderAlgorithmTab();
    void renderGridSizeTab();
    void renderPathsTab();
    void renderAnimationControls();
    void renderResultsTab();
    
    // Agent rendering helpers
    void renderDynamicAgents(ImDrawList* draw_list, const ImVec2& origin, float zoom);
    void renderAgentAtPosition(ImDrawList* draw_list, const Coord& pos, size_t agentId, 
                              const ImVec2& origin, float zoom, bool isGoal = false);
    
    // UI dialog methods
    void renderMapNameDialog();
    void renderScenLoadDialog();
    void renderSaveScenarioDialog();
    void renderProgressDialog();
    void renderParametersDialog();
    void renderCBSParametersDialog();
    void renderCBSProgressDialog();
    void renderCBSResultsDialog();
    
    // CBS methods
    void startCBSSolver();
    void onCBSComplete(const CBSResults& results);

    // LaCAM methods
    void renderLacamParametersDialog();
    void renderLacamProgressDialog();
    void renderLacamResultsDialog();
    void startLacamSolver();
    void onLacamComplete(const LacamResults& results);
    
    // File I/O methods
    void saveMapFile(const std::string& filename);
    void saveScenarioFile(const std::string& filename);
    
    // Agent ID management
    void reassignAgentIDs();
    
    // Temporary file management (private methods)
    std::string getTempDirectory();
    void addTempFile(const std::string& filename);
    bool createTempDirectory();
    void cleanupTempFiles();
    
};