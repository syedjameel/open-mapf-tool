#include "UIManager.h"
#include "GridRenderer.h"
#include "MapFormat.h"
#include <unistd.h> // ::getpid
#include <iostream>
#include <fstream>
#include <cstring>

UIManager::UIManager(Environment& env, PathManager& pathManager, AnimationController& animController)
    : m_env(env), m_pathManager(pathManager), m_animController(animController)
{
    setupFileDialogs();
}

void UIManager::setupStyle()
{
    ImGui::GetStyle().FrameRounding = 4.0f;
    ImGui::GetStyle().GrabRounding = 4.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void UIManager::setupFileDialogs()
{
    m_fileDialog.SetTitle("Load Map");
    m_fileDialog.SetTypeFilters({".map"});
    m_pathsFileDialog.SetTitle("Load Paths");
    m_pathsFileDialog.SetTypeFilters({".txt"});
}

void UIManager::renderSceneWindow(ImDrawList* draw_list, const ImVec2& canvas_pos, const ImVec2& canvas_size, const ImVec2& origin, float zoom)
{
    // Draw grid
    if (m_env.m_square)
    {
        GridRenderer::drawSquareGrid(draw_list, origin, canvas_pos, 
                                    ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                    m_env.m_square_tile_size, zoom, m_env.m_grid);
    }
    else if (m_env.m_hexagon)
    {
        GridRenderer::drawHexGrid(draw_list, origin, zoom, m_env.m_grid, m_env.m_hex_tile_size, m_env.m_hex_grid_type);
    }
    
    // Draw goal markers for agents
    const auto& paths = m_pathManager.getPaths();
    const auto& pathIndices = m_animController.getPathIndices();
    
    // Draw goal markers (if paths are loaded and not in Add/Remove Agents mode)
    if (!paths.empty() && !m_addRemoveAgents)
    {
        for (size_t i = 0; i < paths.size(); ++i)
        {
            const auto& path = paths[i];
            if (path.empty())
                continue;
                
            // Get the agent's goal (last coordinate in path)
            Coord goal = path.back();
            
            // Check if agent has reached its goal
            bool reachedGoal = false;
            if (i < pathIndices.size())
            {
                size_t idx = pathIndices[i];
                if (idx >= path.size() - 1)
                    reachedGoal = true;
            }
            
            // Draw goal marker if agent hasn't reached it yet
            if (!reachedGoal)
            {
                ImVec2 goalPos = GridRenderer::gridToWorld(CoordUtils::x(goal), CoordUtils::y(goal), m_env.m_hexagon,
                                          m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                          origin, zoom, m_env.m_hex_grid_type);
                                          
                // Draw an unfilled square with the same color as the agent
                float markerSize = (m_env.m_hexagon ? m_env.m_hex_agent_size : m_env.m_square_agent_size) * 0.7f * zoom;
                ImVec2 halfSize(markerSize, markerSize);
                draw_list->AddRect(
                    ImVec2(goalPos.x - halfSize.x, goalPos.y - halfSize.y),
                    ImVec2(goalPos.x + halfSize.x, goalPos.y + halfSize.y),
                    getColorForAgent(i),
                    0.0f,
                    0,
                    2.0f * zoom
                );
                
                // Draw agent ID on the goal marker with size proportional to marker size
                char buf[16];
                snprintf(buf, sizeof(buf), "%zu", i);
                
                // Scale text to fit within the marker (about 60% of size)
                float text_scale = markerSize * 0.06f;
                ImGui::GetFont()->Scale = text_scale;
                ImGui::PushFont(ImGui::GetFont());
                
                ImVec2 text_size = ImGui::CalcTextSize(buf);
                
                // Center text properly in the goal marker
                draw_list->AddText(
                    ImVec2(goalPos.x - text_size.x * 0.5f, goalPos.y - text_size.y * 0.5f),
                    IM_COL32(255, 255, 255, 255), buf
                );
                
                // Restore font scale
                ImGui::GetFont()->Scale = 1.0f;
                ImGui::PopFont();
            }
        }
    }
    
    // Draw paths if enabled
    if (m_animController.showPaths())
    {
        const auto& paths = m_pathManager.getPaths();
        const auto& pathIndices = m_animController.getPathIndices();
        float alpha = std::clamp(m_animController.getFrameTimer() / AnimationController::STEP_INTERVAL, 0.0f, 1.0f);
        
        for (size_t i = 0; i < paths.size(); ++i)
        {
            const auto& path = paths[i];
            if (path.size() < 2 || i >= pathIndices.size())
                continue;
                
            size_t idx = pathIndices[i];
            if (idx >= path.size() - 1)
                continue;
                
            ImU32 color = getColorForAgent(i);
            float thickness = 2.0f;
            
            auto [gx1, gy1] = path[idx];
            auto [gx2, gy2] = path[idx + 1];
            
            ImVec2 pos1 = GridRenderer::gridToWorld(gx1, gy1, m_env.m_hexagon,
                                      m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                      origin, zoom, m_env.m_hex_grid_type);
            ImVec2 pos2 = GridRenderer::gridToWorld(gx2, gy2, m_env.m_hexagon,
                                      m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                      origin, zoom, m_env.m_hex_grid_type);
            
            // Interpolated current position
            ImVec2 current = ImVec2(
                pos1.x + (pos2.x - pos1.x) * alpha,
                pos1.y + (pos2.y - pos1.y) * alpha);
                
            // Draw remaining part of the current segment
            draw_list->AddLine(current, pos2, color, thickness);
            
            // Draw full segments beyond the current one
            for (size_t j = idx + 1; j < path.size() - 1; ++j)
            {
                auto [gx_a, gy_a] = path[j];
                auto [gx_b, gy_b] = path[j + 1];
                
                ImVec2 a = GridRenderer::gridToWorld(gx_a, gy_a, m_env.m_hexagon,
                                       m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                       origin, zoom, m_env.m_hex_grid_type);
                ImVec2 b = GridRenderer::gridToWorld(gx_b, gy_b, m_env.m_hexagon,
                                       m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                       origin, zoom, m_env.m_hex_grid_type);
                                       
                draw_list->AddLine(a, b, color, thickness);
            }
        }
    }
    
    // Draw dynamic agents (from Add/Remove Agents mode)
    if (m_addRemoveAgents)
    {
        renderDynamicAgents(draw_list, origin, zoom);
    }
    
    // Draw animated agents (but not when in Add/Remove Agents mode)
    if ((m_animController.isAnimating() || !m_pathManager.getPaths().empty()) && !m_addRemoveAgents)
    {
        const auto& paths = m_pathManager.getPaths();
        const auto& pathIndices = m_animController.getPathIndices();
        float alpha = m_animController.getFrameTimer() / AnimationController::STEP_INTERVAL;
        
        // Ensure we have valid path indices
        if (pathIndices.size() != paths.size()) {
            auto& mutableIndices = const_cast<std::vector<size_t>&>(pathIndices);
            mutableIndices.resize(paths.size(), 0);
        }
        
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (i >= pathIndices.size())
                continue;
                
            const auto& path = paths[i];
            if (path.empty())
                continue;
                
            size_t idx = pathIndices[i];
            if (idx >= path.size())
                idx = path.size() - 1;
                
            Coord p1 = path[idx];
            Coord p2 = (idx + 1 < path.size()) ? path[idx + 1] : p1;
            
            ImVec2 pos1 = GridRenderer::gridToWorld(CoordUtils::x(p1), CoordUtils::y(p1), m_env.m_hexagon,
                                      m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                      origin, zoom, m_env.m_hex_grid_type);
            ImVec2 pos2 = GridRenderer::gridToWorld(CoordUtils::x(p2), CoordUtils::y(p2), m_env.m_hexagon,
                                      m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                      origin, zoom, m_env.m_hex_grid_type);
                                      
            ImVec2 interp_pos = m_animController.isAnimating() ? 
                ImVec2(pos1.x + (pos2.x - pos1.x) * alpha, pos1.y + (pos2.y - pos1.y) * alpha) : pos1;
                
            float agent_radius = (m_env.m_hexagon ? m_env.m_hex_agent_size : m_env.m_square_agent_size) * zoom;
            
            // Draw agent circle with its color
            draw_list->AddCircleFilled(interp_pos, agent_radius, getColorForAgent(i));
            
            // Draw agent ID text centered in the circle with size proportional to agent radius
            char buf[16];
            snprintf(buf, sizeof(buf), "%zu", i);
            
            // Scale text to fit within the circle (about 60% of diameter)
            float text_scale = agent_radius * 0.06f;
            ImGui::GetFont()->Scale = text_scale;
            ImGui::PushFont(ImGui::GetFont());
            
            ImVec2 text_size = ImGui::CalcTextSize(buf);
            draw_list->AddText(ImVec2(interp_pos.x - text_size.x * 0.5f, interp_pos.y - text_size.y * 0.5f), 
                              IM_COL32(255, 255, 255, 255), buf);
            
            // Restore font scale
            ImGui::GetFont()->Scale = 1.0f;
            ImGui::PopFont();
        }
    }
}

void UIManager::renderControlPanel()
{
    ImGui::Begin("Control Panel");
    
    if (ImGui::BeginTabBar("##ControlTabs", ImGuiTabBarFlags_FittingPolicyScroll))
    {
        if (ImGui::BeginTabItem("Map"))
        {
            renderMapTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Algorithm"))
        {
            renderAlgorithmTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Grid & Size"))
        {
            renderGridSizeTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Paths"))
        {
            renderPathsTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Results"))
        {
            renderResultsTab();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::Separator();
    
    // Animation controls
    if (!m_pathManager.getPaths().empty())
    {
        renderAnimationControls();
    }
    
    ImGui::End();
    
    // Render dialogs
    if (m_showMapNameDialog)
    {
        renderMapNameDialog();
    }
    
    if (m_showSaveScenarioDialog)
    {
        renderSaveScenarioDialog();
    }
    
    // Algorithm parameter dialogs
    if (m_selectedAlgo == 0) {
        renderCBSParametersDialog();
        renderCBSProgressDialog();
        renderCBSResultsDialog();
    } else if (m_selectedAlgo >= 1 && m_selectedAlgo <= 3) {
        renderLacamParametersDialog();
        renderLacamProgressDialog();
        renderLacamResultsDialog();
    }
    
}

void UIManager::renderMapTab()
{
    ImGui::Spacing();
    if (ImGui::Button("Load Map"))
    {
        m_fileDialog.Open();
    }
    m_fileDialog.Display();
    
    if (ImGui::Button("Remove Map"))
    {
        m_env.m_grid = Grid<int>(m_env.m_cols, m_env.m_rows, 0);
    }
    
    if (ImGui::Button("Clean Temp Files"))
    {
        cleanupTempFiles();
    }
    
    ImGui::Separator();
    if (ImGui::Checkbox("Add/Remove Obstacles", &m_addRemoveObstacles))
    {
        if (m_addRemoveObstacles)
        {
            m_addRemoveAgents = false;
        }
    }
    
    if (ImGui::Checkbox("Add/Remove Agents", &m_addRemoveAgents))
    {
        if (m_addRemoveAgents)
        {
            m_addRemoveObstacles = false;
            // Clear animation data when entering Add/Remove Agents mode
            // This ensures no stale paths remain from previous algorithm runs
            m_pathManager.clear();
            m_animController.reset();
        }
    }
    
    ImGui::Text("Agents: %zu", m_dynamicAgents.size());
    size_t agentsWithGoals = 0;
    for (const auto& agent : m_dynamicAgents)
    {
        if (agent.hasGoal) agentsWithGoals++;
    }
    ImGui::Text("With Goals: %zu", agentsWithGoals);
    
    // Show agent info and save button when in agent mode or when agents exist
    if (m_addRemoveAgents || m_dynamicAgents.size() > 0)
    {
        ImGui::Separator();
        
        // Clear agents button (show when in agent mode or when agents exist)
        if (m_addRemoveAgents || m_dynamicAgents.size() > 0)
        {
            if (ImGui::Button("Clear Agents"))
            {
                m_dynamicAgents.clear();
                m_nextAgentId = 0;
                m_pendingAgentIndex = SIZE_MAX;
                m_pathManager.clear();
                m_animController.reset();
                // Clean up temporary files when clearing agents
                cleanupTempFiles();
            }
        }
        
        if (m_dynamicAgents.size() > 0)
        {
            if (ImGui::Button("Save Scenario"))
            {
                // Check if we have a map name
                if (m_currentMapName.empty())
                {
                    m_showMapNameDialog = true;
                }
                else
                {
                    m_showSaveScenarioDialog = true;
                }
            }
        }
    }
}

void UIManager::renderAlgorithmTab()
{
    ImGui::Spacing();
    ImGui::Text("Select algorithm:");
    if (ImGui::BeginCombo("##AlgoCombo", m_algoNames[m_selectedAlgo].c_str()))
    {
        for (int n = 0; n < m_algoNames.size(); n++)
        {
            bool is_selected = (m_selectedAlgo == n);
            if (ImGui::Selectable(m_algoNames[n].c_str(), is_selected))
                m_selectedAlgo = n;
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Set Parameters"))
    {
        m_showAlgoParams = true;
    }
    
    ImGui::Separator();
    
    // Show agent count and status
    size_t agentsWithGoals = 0;
    for (const auto& agent : m_dynamicAgents) {
        if (agent.hasGoal) {
            agentsWithGoals++;
        }
    }
    
    ImGui::Text("Agents with goals: %zu", agentsWithGoals);
    
    if (agentsWithGoals == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Add agents with goals to enable solving");
    } else {
        // Handle CBS algorithm
        if (m_selectedAlgo == 0) {  // CBS
            if (m_cbsRunning) {
                if (ImGui::Button("Cancel CBS")) {
                    m_cbsDriver.cancel();
                    m_cbsRunning = false;
                    m_showProgressDialog = false;
                }
                if (m_showProgressDialog) {
                    renderCBSProgressDialog();
                }
            } else {
                if (ImGui::Button("Solve with CBS")) {
                    startCBSSolver();
                }
            }
        }
        // Handle LaCAM algorithms
        else if (m_selectedAlgo >= 1 && m_selectedAlgo <= 3) {  // LaCAM, LaCAM2, LaCAM3
            if (m_lacamRunning) {
                std::string cancelButton = "Cancel " + m_algoNames[m_selectedAlgo];
                if (ImGui::Button(cancelButton.c_str())) {
                    // Cancel the appropriate LaCAM variant
                    if (m_selectedAlgo == 1) m_lacamDriver.cancel();
                    else if (m_selectedAlgo == 2) m_lacam2Driver.cancel();
                    else if (m_selectedAlgo == 3) m_lacam3Driver.cancel();
                    m_lacamRunning = false;
                    m_showProgressDialog = false;
                }
                if (m_showProgressDialog) {
                    renderLacamProgressDialog();
                }
            } else {
                std::string solveButton = "Solve with " + m_algoNames[m_selectedAlgo];
                if (ImGui::Button(solveButton.c_str())) {
                    startLacamSolver();
                }
            }
        }
    }

    // Show results directly in Algorithm tab
    if (m_selectedAlgo == 0 && (m_cbsResults.solutionFound || !m_cbsResults.errorMessage.empty())) {
        ImGui::Separator();
        ImGui::Text("CBS Results:");

        if (m_cbsResults.solutionFound) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Solution Found!");
            ImGui::Text("Solution Cost: %d", m_cbsResults.solutionCost);
            ImGui::Text("Runtime: %.6f seconds", m_cbsResults.runtime);
            ImGui::Text("High Level Nodes: %d", m_cbsResults.highLevelNodes);
            ImGui::Text("Low Level Nodes: %d", m_cbsResults.lowLevelNodes);

            if (!m_cbsResults.pathsFile.empty()) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Paths loaded for visualization");
            }
        } else if (!m_cbsResults.errorMessage.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "✗ Error:");
            ImGui::TextWrapped("%s", m_cbsResults.errorMessage.c_str());
        }
    } else if (m_selectedAlgo >= 1 && m_selectedAlgo <= 3 && (m_lacamResults.solutionFound || !m_lacamResults.errorMessage.empty())) {
        ImGui::Separator();
        ImGui::Text("%s Results:", m_algoNames[m_selectedAlgo].c_str());

        if (m_lacamResults.solutionFound) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Solution Found!");
            ImGui::Text("Sum of Costs: %d (LB: %d)", m_lacamResults.solutionCost, m_lacamResults.socLowerBound);
            ImGui::Text("Makespan: %d (LB: %d)", m_lacamResults.makespan, m_lacamResults.makespanLowerBound);
            ImGui::Text("Runtime: %.6f seconds", m_lacamResults.runtime);
            ImGui::Text("Number of Agents: %d", m_lacamResults.numAgents);

            if (!m_lacamResults.pathsFile.empty()) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Paths loaded for visualization");
            }
        } else if (!m_lacamResults.errorMessage.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "✗ Error:");
            ImGui::TextWrapped("%s", m_lacamResults.errorMessage.c_str());
        }
    }
}

void UIManager::renderGridSizeTab()
{
    ImGui::Spacing();
    ImGui::Text("Grid type:");
    
    bool square = m_env.m_square;
    bool hex = m_env.m_hexagon;
    if (ImGui::RadioButton("Square", square))
    {
        if (!square)
        {
            m_env.m_square = true;
            m_env.m_hexagon = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Hexagonal", hex))
    {
        if (!hex)
        {
            m_env.m_hexagon = true;
            m_env.m_square = false;
        }
    }
    
    // Hex grid type selection
    if (m_env.m_hexagon)
    {
        ImGui::Spacing();
        ImGui::Text("Hex grid layout:");
        
        // Create radio buttons for the four hex grid types
        bool isOddR = (m_env.m_hex_grid_type == HexGridType::ODD_R);
        bool isEvenR = (m_env.m_hex_grid_type == HexGridType::EVEN_R);
        bool isOddQ = (m_env.m_hex_grid_type == HexGridType::ODD_Q);
        bool isEvenQ = (m_env.m_hex_grid_type == HexGridType::EVEN_Q);
        
        if (ImGui::RadioButton("Odd-R (Pointy, odd rows shifted)", isOddR))
            m_env.m_hex_grid_type = HexGridType::ODD_R;
            
        if (ImGui::RadioButton("Even-R (Pointy, even rows shifted)", isEvenR))
            m_env.m_hex_grid_type = HexGridType::EVEN_R;
            
        if (ImGui::RadioButton("Odd-Q (Flat, odd cols shifted)", isOddQ))
            m_env.m_hex_grid_type = HexGridType::ODD_Q;
            
        if (ImGui::RadioButton("Even-Q (Flat, even cols shifted)", isEvenQ))
            m_env.m_hex_grid_type = HexGridType::EVEN_Q;
    }
    
    ImGui::Separator();
    ImGui::Text("Columns: %zu", m_env.m_cols);
    if (ImGui::SliderInt("##ColsSlider", (int*)&m_env.m_cols, 1, 200))
    {
        m_env.m_grid = Grid<int>(m_env.m_cols, m_env.m_rows, 0);
    }
    
    ImGui::Text("Rows: %zu", m_env.m_rows);
    if (ImGui::SliderInt("##RowsSlider", (int*)&m_env.m_rows, 1, 200))
    {
        m_env.m_grid = Grid<int>(m_env.m_cols, m_env.m_rows, 0);
    }
    
    ImGui::Separator();
    ImGui::Text("Square Tile Size: %.2f", m_env.m_square_tile_size);
    ImGui::Text("Hex Tile Size: %.2f", m_env.m_hex_tile_size);
    ImGui::Text("Square Agent Size: %.2f", m_env.m_square_agent_size);
    ImGui::Text("Hex Agent Size: %.2f", m_env.m_hex_agent_size);
}

void UIManager::renderPathsTab()
{
    ImGui::Spacing();
    if (ImGui::Button("Load Paths"))
    {
        m_pathsFileDialog.Open();
    }
    m_pathsFileDialog.Display();
    
    if (ImGui::Button("Remove Paths"))
    {
        m_pathManager.clear();
        m_animController.reset();
    }
    
    ImGui::Separator();
    ImGui::Checkbox("LACAM Format", &m_lacamFormat);
}

void UIManager::renderAnimationControls()
{
    if (ImGui::Button("Start Visualization"))
    {
        m_animController.start(m_pathManager.getPaths());
    }
    
    ImGui::Separator();
    ImGui::Text("Animation");
    
    float speed = m_animController.getAnimationSpeed();
    if (ImGui::SliderFloat("Speed", &speed, 0.1f, 5.0f, "%.1fx"))
    {
        m_animController.setAnimationSpeed(speed);
    }
    
    // Get the current and maximum time steps
    int currentStep = static_cast<int>(m_animController.getCurrentTimeStep());
    int maxStep = static_cast<int>(m_animController.getMaxTimeStep());
    
    // If paths are loaded but controller not initialized, initialize it
    if (maxStep == 0 && !m_pathManager.getPaths().empty()) {
        m_animController.start(m_pathManager.getPaths());
        m_animController.setAnimating(false);
        maxStep = static_cast<int>(m_animController.getMaxTimeStep());
    }
    
    // Create a timeline with actual timesteps
    ImGui::Text("Timeline (Step): %d/%d", currentStep, maxStep);
    
    // Display the slider with step numbers
    if (ImGui::SliderInt("##timeline", &currentStep, 0, maxStep > 0 ? maxStep : 1))
    {
        // Update the controller with the new time step
        m_animController.setCurrentTimeStep(currentStep);
        
        // Update path indices based on time step
        const auto& paths = m_pathManager.getPaths();
        auto& pathIndices = const_cast<std::vector<size_t>&>(m_animController.getPathIndices());
        
        // Ensure pathIndices is properly sized
        if (pathIndices.size() != paths.size()) {
            pathIndices.resize(paths.size(), 0);
        }
        
        // Update each agent's position based on the current time step
        for (size_t i = 0; i < paths.size(); ++i)
        {
            if (paths[i].empty()) continue;
            
            // If current time step is beyond this agent's path length,
            // keep the agent at its final position
            size_t pathLength = paths[i].size();
            if (currentStep >= pathLength - 1) {
                pathIndices[i] = pathLength - 1;
            } else {
                // Otherwise, set the agent's position to the current time step
                pathIndices[i] = currentStep;
            }
        }
        
        m_animController.setAnimating(false);
    }
    
    bool showPaths = m_animController.showPaths();
    if (ImGui::Checkbox("Show Agent Paths", &showPaths))
    {
        m_animController.setShowPaths(showPaths);
    }
    
    if (ImGui::Button("Reset Animation"))
    {
        m_animController.reset();
    }
}

void UIManager::handleFileDialogs()
{
    if (m_fileDialog.HasSelected())
    {
        try
        {
            auto path = m_fileDialog.GetSelected().string();
            m_env.loadMap(path);
            m_recentFiles.push_back(path);
            m_currentMapPath = path;
            // Clean up temporary files when loading new map
            cleanupTempFiles();
            
            // Extract map name from path for scenario saving
            size_t lastSlash = path.find_last_of("/\\");
            size_t lastDot = path.find_last_of(".");
            if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash)
            {
                m_currentMapName = path.substr(lastSlash + 1, lastDot - lastSlash - 1);
            }
            else if (lastDot != std::string::npos)
            {
                m_currentMapName = path.substr(0, lastDot);
            }
            else
            {
                m_currentMapName = path;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading map: " << e.what() << std::endl;
        }
        m_fileDialog.ClearSelected();
    }
    
    if (m_pathsFileDialog.HasSelected())
    {
        try
        {
            auto path = m_pathsFileDialog.GetSelected().string();
            m_pathManager.loadPaths(path, m_lacamFormat);
            m_currentPathFile = path;
            
            // Initialize the animation controller with the loaded paths
            // This ensures the max time step is calculated before visualization starts
            m_animController.start(m_pathManager.getPaths());
            m_animController.setAnimating(false); // Don't start animation yet
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading paths: " << e.what() << std::endl;
        }
        m_pathsFileDialog.ClearSelected();
    }
}

void UIManager::handleAgentInteraction(const ImVec2& mousePos, const ImVec2& origin, float zoom, bool isHovered, bool isLeftClicked)
{
    if (!isHovered || !isLeftClicked || !m_addRemoveAgents)
        return;

    float tileSize = m_env.m_square ? m_env.m_square_tile_size : m_env.m_hex_tile_size;
    float scaledTile = tileSize * zoom;

    size_t col = 0, row = 0;

    if (m_env.m_square)
    {
        float mouseX = (mousePos.x - origin.x + (scaledTile * 0.5f)) / scaledTile;
        float mouseY = (mousePos.y - origin.y + (scaledTile * 0.5f)) / scaledTile;
        col = static_cast<size_t>(mouseX);
        row = static_cast<size_t>(mouseY);
    }
    else
    {
        // Convert ImGui mouse to grid-space in pixels (before hex conversion)
        ImVec2 worldMousePos = {
            (mousePos.x - origin.x) / zoom,
            (mousePos.y - origin.y) / zoom};

        Coord hexCoord = GridRenderer::pixelToHex(worldMousePos, tileSize, m_env.m_hex_grid_type);
        col = static_cast<size_t>(CoordUtils::x(hexCoord));
        row = static_cast<size_t>(CoordUtils::y(hexCoord));
    }

    if (col >= m_env.m_cols || row >= m_env.m_rows)
        return;

    Coord clickedPos = CoordUtils::make_coord(col, row);
    
    // Check if there's blocked terrain at this position
    if (MapFormat::isBlocked(m_env.m_grid.get(col, row)))
        return; // Can't place agent on blocked cell

    // Check if we're clicking on an existing agent
    for (size_t i = 0; i < m_dynamicAgents.size(); ++i)
    {
        Agent& agent = m_dynamicAgents[i];
        
        // Check if clicking on agent's start position
        if (CoordUtils::x(agent.start) == col && CoordUtils::y(agent.start) == row)
        {
            // Remove this agent
            m_dynamicAgents.erase(m_dynamicAgents.begin() + i);
            // Reset pending agent if it was this one
            if (m_pendingAgentIndex == i)
            {
                m_pendingAgentIndex = SIZE_MAX;
            }
            else if (m_pendingAgentIndex > i)
            {
                m_pendingAgentIndex--;
            }
            // Reassign IDs to maintain ID = array index consistency
            reassignAgentIDs();
            // Clear animation data to remove stale paths/goals
            m_pathManager.clear();
            m_animController.reset();
            return;
        }
        
        // Check if clicking on agent's goal position
        if (agent.hasGoal && CoordUtils::x(agent.goal) == col && CoordUtils::y(agent.goal) == row)
        {
            // Remove goal, make this agent pending again
            agent.hasGoal = false;
            agent.goal = CoordUtils::make_coord(-1, -1);
            m_pendingAgentIndex = i;
            return;
        }
    }

    // If we have a pending agent (waiting for goal), set its goal
    if (m_pendingAgentIndex != SIZE_MAX && m_pendingAgentIndex < m_dynamicAgents.size())
    {
        Agent& agent = m_dynamicAgents[m_pendingAgentIndex];
        
        // Check if clicking on the same position as start
        if (CoordUtils::x(agent.start) == col && CoordUtils::y(agent.start) == row)
        {
            // Remove this agent (clicking on same cell)
            m_dynamicAgents.erase(m_dynamicAgents.begin() + m_pendingAgentIndex);
            m_pendingAgentIndex = SIZE_MAX;
            // Reassign IDs to maintain ID = array index consistency
            reassignAgentIDs();
            // Clear animation data to remove stale paths/goals
            m_pathManager.clear();
            m_animController.reset();
            return;
        }
        
        // Set goal for pending agent
        agent.goal = clickedPos;
        agent.hasGoal = true;
        m_pendingAgentIndex = SIZE_MAX;
    }
    else
    {
        // Create new agent at clicked position
        // ID will be set by array position to maintain consistency
        size_t newId = m_dynamicAgents.size();
        Agent newAgent(newId, clickedPos);
        m_dynamicAgents.push_back(newAgent);
        m_pendingAgentIndex = m_dynamicAgents.size() - 1;
    }
}

void UIManager::renderDynamicAgents(ImDrawList* draw_list, const ImVec2& origin, float zoom)
{
    for (size_t i = 0; i < m_dynamicAgents.size(); ++i)
    {
        const Agent& agent = m_dynamicAgents[i];
        
        // Render agent start position
        renderAgentAtPosition(draw_list, agent.start, agent.id, origin, zoom, false);
        
        // Render goal position if it exists
        if (agent.hasGoal)
        {
            renderAgentAtPosition(draw_list, agent.goal, agent.id, origin, zoom, true);
        }
    }
}

void UIManager::renderAgentAtPosition(ImDrawList* draw_list, const Coord& pos, size_t agentId, 
                                     const ImVec2& origin, float zoom, bool isGoal)
{
    ImVec2 worldPos = GridRenderer::gridToWorld(CoordUtils::x(pos), CoordUtils::y(pos), m_env.m_hexagon,
                                               m_env.m_hexagon ? m_env.m_hex_tile_size : m_env.m_square_tile_size,
                                               origin, zoom, m_env.m_hex_grid_type);
    
    float agentSize = (m_env.m_hexagon ? m_env.m_hex_agent_size : m_env.m_square_agent_size) * zoom;
    ImU32 color = getColorForAgent(agentId);
    
    if (isGoal)
    {
        // Draw goal as unfilled square
        float markerSize = agentSize * 0.7f;
        ImVec2 halfSize(markerSize, markerSize);
        draw_list->AddRect(
            ImVec2(worldPos.x - halfSize.x, worldPos.y - halfSize.y),
            ImVec2(worldPos.x + halfSize.x, worldPos.y + halfSize.y),
            color,
            0.0f,
            0,
            2.0f * zoom
        );
    }
    else
    {
        // Draw agent as filled circle
        draw_list->AddCircleFilled(worldPos, agentSize, color);
    }
    
    // Draw agent ID text
    char buf[16];
    snprintf(buf, sizeof(buf), "%zu", agentId);
    
    float text_scale = agentSize * 0.06f;
    ImGui::GetFont()->Scale = text_scale;
    ImGui::PushFont(ImGui::GetFont());
    
    ImVec2 text_size = ImGui::CalcTextSize(buf);
    draw_list->AddText(
        ImVec2(worldPos.x - text_size.x * 0.5f, worldPos.y - text_size.y * 0.5f),
        IM_COL32(255, 255, 255, 255), buf
    );
    
    ImGui::GetFont()->Scale = 1.0f;
    ImGui::PopFont();
}

void UIManager::renderMapNameDialog()
{
    ImGui::OpenPopup("Map Name Required");
    
    if (ImGui::BeginPopupModal("Map Name Required", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Please enter a name for the map:");
        ImGui::Separator();
        
        ImGui::InputText("Map Name", m_mapNameBuffer, sizeof(m_mapNameBuffer));
        
        ImGui::Separator();
        
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            if (strlen(m_mapNameBuffer) > 0)
            {
                m_currentMapName = std::string(m_mapNameBuffer);
                m_showMapNameDialog = false;
                m_showSaveScenarioDialog = true;
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_showMapNameDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void UIManager::renderSaveScenarioDialog()
{
    ImGui::OpenPopup("Save Scenario");
    
    if (ImGui::BeginPopupModal("Save Scenario", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save scenario files for map: %s", m_currentMapName.c_str());
        ImGui::Text("Agents: %zu", m_dynamicAgents.size());
        
        size_t agentsWithGoals = 0;
        for (const auto& agent : m_dynamicAgents)
        {
            if (agent.hasGoal) agentsWithGoals++;
        }
        ImGui::Text("Agents with goals: %zu", agentsWithGoals);
        
        ImGui::Separator();
        
        // Option for temporary vs permanent files
        ImGui::Checkbox("Use temporary files (auto-cleanup)", &m_useTemporaryFiles);
        ImGui::Text("Temporary files are cleaned up automatically when:");
        ImGui::BulletText("Program exits");
        ImGui::BulletText("Clear Agents is pressed");
        ImGui::BulletText("New map is loaded");
        
        ImGui::Separator();
        
        if (ImGui::Button("Save Map + Scenario", ImVec2(200, 0)))
        {
            try
            {
                std::string mapFile, scenFile;
                
                if (m_useTemporaryFiles)
                {
                    // Use temporary files in temp directory
                    std::string tempDir = getTempDirectory();
                    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    std::string prefix = tempDir + "/" + m_currentMapName + "_" + std::to_string(timestamp);
                    
                    mapFile = prefix + ".map";
                    scenFile = prefix + ".scen";
                    
                    // Track temp files for cleanup
                    addTempFile(mapFile);
                    addTempFile(scenFile);
                }
                else
                {
                    // Use permanent files in current directory
                    mapFile = m_currentMapName + ".map";
                    scenFile = m_currentMapName + ".scen";
                }
                
                saveMapFile(mapFile);
                saveScenarioFile(scenFile);
                m_showSaveScenarioDialog = false;
                ImGui::CloseCurrentPopup();
                
                // If CBS was requested, start it now
                if (m_startCBSAfterSave) {
                    m_startCBSAfterSave = false;
                    // Call startCBSSolver recursively, but now m_currentMapName is set
                    startCBSSolver();
                }
                // If LaCAM was requested, start it now
                if (m_startLacamAfterSave) {
                    m_startLacamAfterSave = false;
                    // Call startLacamSolver recursively, but now m_currentMapName is set
                    startLacamSolver();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error saving files: " << e.what() << std::endl;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_showSaveScenarioDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void UIManager::saveMapFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    MapFormat::writeMap(file, m_env.m_grid, m_env.m_cols, m_env.m_rows);
    file.close();
    std::cout << "Map saved to: " << filename << std::endl;
}

void UIManager::saveScenarioFile(const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Extract just the map filename (without path)
    std::string mapFilename = m_currentMapName + ".map";
    MapFormat::writeScen(file, m_dynamicAgents, mapFilename, m_env.m_cols, m_env.m_rows);
    file.close();
    std::cout << "Scenario saved to: " << filename << std::endl;
}

void UIManager::reassignAgentIDs()
{
    // Reassign agent IDs to be consecutive starting from 0
    for (size_t i = 0; i < m_dynamicAgents.size(); ++i)
    {
        m_dynamicAgents[i].id = i;
    }
    
    // Update m_nextAgentId to be the next available ID
    m_nextAgentId = m_dynamicAgents.size();
}


void UIManager::renderResultsTab()
{
    ImGui::Spacing();
    if (m_cbsResults.solutionFound) {
        ImGui::Text("CBS Results:");
        ImGui::Separator();
        ImGui::Text("Solution Cost: %d", m_cbsResults.solutionCost);
        ImGui::Text("Runtime: %.3f seconds", m_cbsResults.runtime);
        ImGui::Text("High Level Nodes: %d", m_cbsResults.highLevelNodes);
        ImGui::Text("Low Level Nodes: %d", m_cbsResults.lowLevelNodes);
        
        if (!m_cbsResults.pathsFile.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Paths loaded for visualization");
        }
    } else if (!m_cbsResults.errorMessage.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "CBS Error:");
        ImGui::TextWrapped("%s", m_cbsResults.errorMessage.c_str());
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No algorithm results yet.");
        ImGui::Text("Run an algorithm to see detailed statistics.");
    }
}

void UIManager::startCBSSolver()
{
    // If already running, don't start another instance
    if (m_cbsRunning) {
        return;
    }
    
    // Make sure any previous thread is cleaned up
    if (m_cbsDriver.isRunning()) {
        m_cbsDriver.cancel();
        // Wait a bit for thread to finish
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (m_currentMapName.empty()) {
        m_startCBSAfterSave = true;  // Flag to start CBS after saving
        m_showMapNameDialog = true;
        return;
    }
    
    // Update CBS parameters from environment
    m_cbsParams.hexGrid = m_env.m_hexagon;
    if (m_env.m_hexagon) {
        switch (m_env.m_hex_grid_type) {
            case HexGridType::ODD_R: m_cbsParams.gridConfig = "oddr"; break;
            case HexGridType::EVEN_R: m_cbsParams.gridConfig = "evenr"; break;
            case HexGridType::ODD_Q: m_cbsParams.gridConfig = "oddq"; break;
            case HexGridType::EVEN_Q: m_cbsParams.gridConfig = "evenq"; break;
        }
    }
    
    // CBS works directly with m_dynamicAgents
    
    m_cbsRunning = true;
    m_cbsStartTime = std::chrono::steady_clock::now();
    m_cbsResults = CBSResults(); // Reset results
    
    // Solver executable comes from the config (empty = not configured;
    // the driver reports a clear error instead of running)
    m_cbsDriver.setExecutablePath(m_env.m_solver_cbs);

    // Use temp directory and register temp files for cleanup
    std::string tempDir = getTempDirectory();
    m_cbsDriver.solveAsync(m_dynamicAgents, m_env, m_cbsParams, m_currentMapName,
                          [this](const CBSResults& results) {
                              this->onCBSComplete(results);
                          },
                          tempDir,
                          [this](const std::string& filename) {
                              this->addTempFile(filename);
                          });
}

void UIManager::onCBSComplete(const CBSResults& results)
{
    // Make sure we're still expecting results (avoid race conditions)
    if (!m_cbsRunning) {
        return;
    }
    
    m_cbsResults = results;
    m_cbsRunning = false;
    m_showProgressDialog = false;
    
    // Auto-load paths immediately if solution found
    if (m_cbsResults.solutionFound && !m_cbsResults.pathsFile.empty()) {
        try {
            // Use existing path loader (same as Load Paths button)
            m_pathManager.loadPaths(m_cbsResults.pathsFile, m_lacamFormat);
            
            // Initialize animation controller (same as existing mechanism)
            m_animController.start(m_pathManager.getPaths());
            m_animController.setAnimating(true);  // Start animation automatically

            // Exit Add/Remove Agents mode to ensure clean visualization
            m_addRemoveAgents = false;
            
            std::cout << "CBS paths loaded automatically for visualization" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error auto-loading CBS paths: " << e.what() << std::endl;
        }
    }
    
    // Don't show popup - results will be shown in Algorithm tab
    // m_showResultsPanel = true;
}

void UIManager::renderCBSParametersDialog()
{
    if (!m_showAlgoParams) return;
    
    ImGui::OpenPopup("CBS Parameters");
    
    if (ImGui::BeginPopupModal("CBS Parameters", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("CBS Algorithm Parameters");
        ImGui::Separator();
        
        ImGui::InputInt("Cutoff Time (seconds)", &m_cbsParams.cutoffTime);
        
        // Heuristic selection
        const char* heuristics[] = {"Zero", "CG", "DG", "WDG"};
        int currentHeuristic = 0;
        for (int i = 0; i < 4; i++) {
            if (m_cbsParams.heuristic == heuristics[i]) {
                currentHeuristic = i;
                break;
            }
        }
        if (ImGui::Combo("Heuristic", &currentHeuristic, heuristics, 4)) {
            m_cbsParams.heuristic = heuristics[currentHeuristic];
        }
        
        // Cluster heuristics
        const char* clusterHeuristics[] = {"N", "BP", "CH", "CHBPNS", "CHBPNM", "CHBPNRM", "CHBP"};
        int currentClusterHeuristic = 0;
        for (int i = 0; i < 7; i++) {
            if (m_cbsParams.clusterHeuristic == clusterHeuristics[i]) {
                currentClusterHeuristic = i;
                break;
            }
        }
        if (ImGui::Combo("Cluster Heuristic", &currentClusterHeuristic, clusterHeuristics, 7)) {
            m_cbsParams.clusterHeuristic = clusterHeuristics[currentClusterHeuristic];
        }
        
        // Rectangle reasoning
        const char* rectangleOptions[] = {"None", "R", "RM", "GR", "Disjoint"};
        int currentRectangle = 0;
        for (int i = 0; i < 5; i++) {
            if (m_cbsParams.rectangleReasoning == rectangleOptions[i]) {
                currentRectangle = i;
                break;
            }
        }
        if (ImGui::Combo("Rectangle Reasoning", &currentRectangle, rectangleOptions, 5)) {
            m_cbsParams.rectangleReasoning = rectangleOptions[currentRectangle];
        }
        
        // Corridor reasoning
        const char* corridorOptions[] = {"None", "C", "PC", "STC", "GC", "Disjoint"};
        int currentCorridor = 0;
        for (int i = 0; i < 6; i++) {
            if (m_cbsParams.corridorReasoning == corridorOptions[i]) {
                currentCorridor = i;
                break;
            }
        }
        if (ImGui::Combo("Corridor Reasoning", &currentCorridor, corridorOptions, 6)) {
            m_cbsParams.corridorReasoning = corridorOptions[currentCorridor];
        }
        
        ImGui::Checkbox("Bypass", &m_cbsParams.bypass);
        ImGui::Checkbox("Target Reasoning", &m_cbsParams.targetReasoning);
        ImGui::Checkbox("Mutex Reasoning", &m_cbsParams.mutexReasoning);
        
        ImGui::Separator();
        
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_showAlgoParams = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showAlgoParams = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void UIManager::renderCBSProgressDialog()
{
    if (!m_showProgressDialog || !m_cbsRunning) return;
    
    ImGui::OpenPopup("CBS Solving");
    
    if (ImGui::BeginPopupModal("CBS Solving", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("CBS solver is running...");
        
        // Calculate elapsed time
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_cbsStartTime);
        ImGui::Text("Elapsed time: %ld seconds", elapsed.count());
        
        // Spinner animation
        static float progress = 0.0f;
        progress += 0.01f;
        if (progress >= 1.0f) progress = 0.0f;
        
        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
        
        if (ImGui::Button("Cancel")) {
            m_cbsDriver.cancel();
            m_cbsRunning = false;
            m_showProgressDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        if (!m_cbsRunning) {
            m_showProgressDialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void UIManager::renderCBSResultsDialog()
{
    if (!m_showResultsPanel) return;
    
    ImGui::OpenPopup("CBS Results");
    
    if (ImGui::BeginPopupModal("CBS Results", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (m_cbsResults.solutionFound) {
            ImGui::Text("Solution Found!");
            ImGui::Separator();
            ImGui::Text("Solution Cost: %d", m_cbsResults.solutionCost);
            ImGui::Text("Runtime: %.3f seconds", m_cbsResults.runtime);
            ImGui::Text("High Level Nodes: %d", m_cbsResults.highLevelNodes);
            ImGui::Text("Low Level Nodes: %d", m_cbsResults.lowLevelNodes);
            
            if (!m_cbsResults.pathsFile.empty()) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ Paths automatically loaded for visualization");
                ImGui::Text("You can now use the animation controls to view the solution.");
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No Solution Found");
            if (!m_cbsResults.errorMessage.empty()) {
                ImGui::Separator();
                ImGui::TextWrapped("Error: %s", m_cbsResults.errorMessage.c_str());
            }
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            m_showResultsPanel = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

// Destructor - cleanup temporary files
UIManager::~UIManager()
{
    cleanupTempFiles();
}

// Temporary file management implementation
std::string UIManager::getTempDirectory()
{
    if (m_tempDirPath.empty())
    {
        // Per-process directory under the system temp dir: independent of
        // the current working directory and safe from clashing with (or
        // deleting) user files.
        m_tempDirPath = (std::filesystem::temp_directory_path() /
                         ("openmapf-" + std::to_string(::getpid()))).string();
    }
    
    // Always ensure temp directory exists when requested
    if (!std::filesystem::exists(m_tempDirPath))
    {
        createTempDirectory();
    }
    
    return m_tempDirPath;
}

bool UIManager::createTempDirectory()
{
    try
    {
        std::filesystem::create_directories(m_tempDirPath);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to create temp directory: " << e.what() << std::endl;
        return false;
    }
}

void UIManager::addTempFile(const std::string& filename)
{
    m_tempFiles.push_back(filename);
}

void UIManager::cleanupTempFiles()
{
    // Clean up tracked temp files
    for (const auto& file : m_tempFiles)
    {
        try
        {
            if (std::filesystem::exists(file))
            {
                std::filesystem::remove(file);
                std::cout << "Cleaned up temp file: " << file << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to delete temp file " << file << ": " << e.what() << std::endl;
        }
    }
    m_tempFiles.clear();

    // NOTE: this used to also scan the *current working directory* and
    // delete any file matching *_<digits>.{map,scen,csv,txt}, which could
    // silently destroy a user's own files (e.g. experiment_1.map). Cleanup
    // is now strictly limited to the tracked temp files above.
    // (Plan defect A3)

    // Keep temp directory available (don't remove it)
    // This ensures CBS can always create files when needed
    try
    {
        if (!m_tempDirPath.empty() && !std::filesystem::exists(m_tempDirPath))
        {
            std::filesystem::create_directories(m_tempDirPath);
            std::cout << "Recreated temp directory: " << m_tempDirPath << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to recreate temp directory: " << e.what() << std::endl;
    }
}

// LaCAM Methods Implementation
void UIManager::startLacamSolver()
{
    // If already running, don't start another instance
    if (m_lacamRunning) {
        return;
    }

    // Check if we have dynamic agents with goals
    size_t agentsWithGoals = 0;
    for (const auto& agent : m_dynamicAgents) {
        if (agent.hasGoal) {
            agentsWithGoals++;
        }
    }

    if (agentsWithGoals == 0) {
        return;
    }

    // If no map name set, prompt for one
    if (m_currentMapName.empty()) {
        m_startLacamAfterSave = true;
        m_showMapNameDialog = true;
        return;
    }

    m_lacamRunning = true;
    m_showProgressDialog = true;
    m_lacamStartTime = std::chrono::steady_clock::now();

    // Select the appropriate driver based on selected algorithm and inject
    // its executable path from the config (empty = not configured; the
    // driver reports a clear error instead of running)
    LacamDriverBase* driver = nullptr;
    if (m_selectedAlgo == 1) {
        driver = &m_lacamDriver;
        driver->setExecutablePath(m_env.m_solver_lacam);
    } else if (m_selectedAlgo == 2) {
        driver = &m_lacam2Driver;
        driver->setExecutablePath(m_env.m_solver_lacam2);
    } else if (m_selectedAlgo == 3) {
        driver = &m_lacam3Driver;
        driver->setExecutablePath(m_env.m_solver_lacam3);
    } else {
        m_lacamRunning = false;
        return;
    }

    // Use temp directory and register temp files for cleanup
    std::string tempDir = getTempDirectory();
    driver->solveAsync(m_dynamicAgents, m_env, m_lacamParams, m_currentMapName,
                       [this](const LacamResults& results) {
                           this->onLacamComplete(results);
                       },
                       tempDir,
                       [this](const std::string& filename) {
                           this->addTempFile(filename);
                       });
}

void UIManager::onLacamComplete(const LacamResults& results)
{
    // Make sure we're still expecting results (avoid race conditions)
    if (!m_lacamRunning) {
        return;
    }

    m_lacamRunning = false;
    m_showProgressDialog = false;
    m_lacamResults = results;

    // If solution found, load the paths
    if (results.solutionFound && !results.pathsFile.empty()) {
        try {
            // Load paths for visualization
            // Pass true for lacamFormat to prevent coordinate swapping since we already handle it
            m_pathManager.loadPaths(results.pathsFile, true);  // Already in correct format after conversion

            // Start animation
            m_animController.start(m_pathManager.getPaths());
            m_animController.setAnimating(true);  // Make sure animation actually starts

            // Exit Add/Remove Agents mode to show animated agents
            m_addRemoveAgents = false;

            std::cout << "[" << m_algoNames[m_selectedAlgo] << "] Solution found! Cost: "
                     << results.solutionCost << ", Makespan: " << results.makespan
                     << ", Runtime: " << results.runtime << "s" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[" << m_algoNames[m_selectedAlgo] << "] Failed to load paths: "
                     << e.what() << std::endl;
            m_lacamResults.errorMessage = "Solution found but failed to load paths: "
                                        + std::string(e.what());
        }
    } else {
        std::cerr << "[" << m_algoNames[m_selectedAlgo] << "] No solution found or error: "
                 << results.errorMessage << std::endl;
    }
}

void UIManager::renderLacamParametersDialog()
{
    if (!m_showAlgoParams) return;
    if (m_selectedAlgo < 1 || m_selectedAlgo > 3) return;  // Only for LaCAM algorithms

    std::string popupTitle = m_algoNames[m_selectedAlgo] + " Parameters";
    ImGui::OpenPopup(popupTitle.c_str());

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(popupTitle.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Configure %s parameters:", m_algoNames[m_selectedAlgo].c_str());
        ImGui::Separator();

        // Core parameters
        ImGui::SliderInt("Time Limit (seconds)", &m_lacamParams.timeLimit, 1, 300);
        ImGui::SliderInt("Random Seed", &m_lacamParams.seed, 0, 10000);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Random seed for reproducible results");

        ImGui::SliderInt("Verbose Level", &m_lacamParams.verbose, 0, 3);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("0: silent, 1: basic, 2: detailed, 3: debug");

        ImGui::Checkbox("Log Short Format", &m_lacamParams.logShort);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use short log format for output");

        // Grid configuration (read-only, from environment)
        ImGui::Separator();
        ImGui::Text("Grid Configuration:");
        if (m_env.m_hexagon) {
            ImGui::Text("  Type: Hexagonal");
            std::string hexConfig = "odd-row";  // Default
            switch (m_env.m_hex_grid_type) {
                case HexGridType::ODD_R: hexConfig = "odd-row"; break;
                case HexGridType::EVEN_R: hexConfig = "even-row"; break;
                case HexGridType::ODD_Q: hexConfig = "odd-column"; break;
                case HexGridType::EVEN_Q: hexConfig = "even-column"; break;
            }
            ImGui::Text("  Config: %s", hexConfig.c_str());
        } else {
            ImGui::Text("  Type: Square");
        }

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            m_showAlgoParams = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            m_showAlgoParams = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderLacamProgressDialog()
{
    if (!m_showProgressDialog || !m_lacamRunning) return;

    ImGui::OpenPopup("LaCAM Progress");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("LaCAM Progress", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s is running...", m_algoNames[m_selectedAlgo].c_str());

        // Calculate elapsed time
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lacamStartTime).count() / 1000.0f;

        ImGui::Text("Elapsed time: %.2f seconds", elapsed);

        // Progress bar (indeterminate)
        static float progress = 0.0f;
        progress += ImGui::GetIO().DeltaTime * 0.2f;
        if (progress > 1.0f) progress = 0.0f;

        ImGui::ProgressBar(progress, ImVec2(300, 0), "");

        ImGui::Separator();

        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            // Cancel the appropriate driver
            if (m_selectedAlgo == 1) m_lacamDriver.cancel();
            else if (m_selectedAlgo == 2) m_lacam2Driver.cancel();
            else if (m_selectedAlgo == 3) m_lacam3Driver.cancel();

            m_lacamRunning = false;
            m_showProgressDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::renderLacamResultsDialog()
{
    // Results are shown directly in the Algorithm tab
    // This method is kept for consistency but not used currently
}

