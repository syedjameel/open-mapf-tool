#pragma once

#include "imgui.h"
#include "imfilebrowser.h"
#include "Environment.h"
#include "PathManager.h"
#include "AnimationController.h"
#include "Types.h"
#include "CBSDriver.h"
#include <vector>
#include <string>
#include <memory>
#include <chrono>

class UIManager {
public:
    UIManager(Environment& env, PathManager& pathManager, AnimationController& animController);
    
    void setupStyle();
    void setupFileDialogs();
    
    void renderSceneWindow(ImDrawList* draw_list, const ImVec2& canvas_pos, const ImVec2& canvas_size, const ImVec2& origin, float zoom);
    void renderControlPanel();
    
    void handleFileDialogs();
    
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
    
    // References to other components
    Environment& m_env;
    PathManager& m_pathManager;
    AnimationController& m_animController;
    
    // Algorithm selection
    std::vector<std::string> m_algoNames = {"CBS"};
    int m_selectedAlgo = 0;
    bool m_showAlgoParams = false;
    
    // CBS integration
    CBSDriver m_cbsDriver;
    CBSParameters m_cbsParams;
    CBSResults m_cbsResults;
    bool m_cbsRunning = false;
    bool m_startCBSAfterSave = false;
    std::chrono::steady_clock::time_point m_cbsStartTime;
    
    // Recent files
    std::vector<std::string> m_recentFiles;
    std::string m_currentMapPath;
    std::string m_currentPathFile;
    
    // UI rendering methods
    void renderMapTab();
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
    void renderSaveScenarioDialog();
    void renderProgressDialog();
    void renderParametersDialog();
    void renderCBSParametersDialog();
    void renderCBSProgressDialog();
    void renderCBSResultsDialog();
    
    // CBS methods
    void startCBSSolver();
    void onCBSComplete(const CBSResults& results);
    
    // File I/O methods
    void saveMapFile(const std::string& filename);
    void saveScenarioFile(const std::string& filename);
    
    // Agent ID management
    void reassignAgentIDs();
    
};