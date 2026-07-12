#pragma once
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Environment.h"
#include "PathManager.h"
#include "AnimationController.h"
#include "UIManager.h"
#include "ViewportManager.h"
#include "GridInteractionHandler.h"

class ImGuiMapfApp
{
public:
    void run();

private:
    // Core components
    Environment m_env{"../config/env.yaml"};
    PathManager m_pathManager;
    AnimationController m_animController;
    UIManager m_uiManager{m_env, m_pathManager, m_animController};
    ViewportManager m_viewportManager;
    GridInteractionHandler m_gridInteractionHandler{m_env};
    
    // Setup methods
    void setupImGui(GLFWwindow* window, const char* glslVersion);
    void cleanupImGui(GLFWwindow* window);
    
    // Main loop methods
    void processFrame(GLFWwindow* window);
    void renderScene();
};