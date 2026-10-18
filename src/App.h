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

    // Files to open when the window appears (all optional): a MovingAI map,
    // a grid type ("square", "hex-oddr", ...), and a LaCAM output file whose
    // solution is loaded and played.
    struct StartupFiles
    {
        std::string map;
        std::string grid;
        std::string plan;
    };
    void setStartupFiles(StartupFiles files) { m_startup = std::move(files); }

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
    ImVec2 m_lastCanvasSize{0.0f, 0.0f};
    StartupFiles m_startup;
    bool m_layoutChecked = false; // first-start dock layout applied (or a saved one found)
    double m_lastWorkMs = 0.0;    // time spent building and submitting the last frame (before swap)

    // Setup methods
    void setupImGui(GLFWwindow* window, const char* glslVersion);
    void cleanupImGui(GLFWwindow* window);

    // Opens m_startup; false (with a message on stderr) if a file cannot be used
    bool openStartupFiles();

    // Main loop methods
    void processFrame(GLFWwindow* window);
    void renderScene();
    void fitViewToGrid(const ImVec2& canvasSize, bool immediate);
};