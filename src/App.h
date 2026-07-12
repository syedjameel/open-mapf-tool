#pragma once
#include "AnimationController.h"
#include "Environment.h"
#include "GridInteractionHandler.h"
#include "PathManager.h"
#include "SolverManifest.h"
#include "UIManager.h"
#include "ViewportManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

class ImGuiMapfApp
{
public:
    // configPath / solversPath: resolved config file paths ("" = defaults).
    explicit ImGuiMapfApp(const std::string& configPath = "", const std::string& solversPath = "")
        : m_env(configPath)
    {
        m_solverManifest = SolverManifest::withDefaults();
        // Precedence: solvers.yaml wins; legacy env.yaml 'solvers' keys
        // fill entries the manifest leaves unconfigured.
        m_solverManifest.applyLegacyEnvironment(m_env);
        if (!solversPath.empty()) {
            m_solverManifest.loadFromFile(solversPath);
        }
    }

    // Returns a process exit code (0 on success).
    int run();

private:
    // Core components
    Environment m_env;
    SolverManifest m_solverManifest;
    PathManager m_pathManager;
    AnimationController m_animController;
    UIManager m_uiManager{m_env, m_pathManager, m_animController, m_solverManifest};
    ViewportManager m_viewportManager;
    GridInteractionHandler m_gridInteractionHandler{m_env};

    // Setup methods
    void setupImGui(GLFWwindow* window, const char* glslVersion);
    void cleanupImGui(GLFWwindow* window);

    // Main loop methods
    void processFrame(GLFWwindow* window);
    void renderScene();
};