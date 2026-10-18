#include "App.h"
#include "GridRenderer.h"
#include "SolutionParsers.h"
#include "Version.h"
#include "imgui_internal.h"
#include "implot.h"
#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace {
// Grid name as used in experiment specs and results.csv
const char* gridName(const Environment& env)
{
    if (!env.m_hexagon) {
        return "square";
    }
    switch (env.m_hex_grid_type) {
    case HexGridType::ODD_R:
        return "hex-oddr";
    case HexGridType::EVEN_R:
        return "hex-evenr";
    case HexGridType::ODD_Q:
        return "hex-oddq";
    case HexGridType::EVEN_Q:
        return "hex-evenq";
    }
    return "hex";
}
} // namespace

bool ImGuiMapfApp::openStartupFiles()
{
    try {
        if (!m_startup.map.empty()) {
            m_env.loadMap(m_startup.map);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading map: " << e.what() << std::endl;
        return false;
    }
    const std::string& grid = m_startup.grid;
    if (!grid.empty()) {
        m_env.m_square = grid == "square";
        m_env.m_hexagon = !m_env.m_square;
        if (grid == "hex-oddr")
            m_env.m_hex_grid_type = HexGridType::ODD_R;
        else if (grid == "hex-evenr")
            m_env.m_hex_grid_type = HexGridType::EVEN_R;
        else if (grid == "hex-oddq")
            m_env.m_hex_grid_type = HexGridType::ODD_Q;
        else if (grid == "hex-evenq")
            m_env.m_hex_grid_type = HexGridType::EVEN_Q;
    }
    if (!m_startup.plan.empty()) {
        // Same conversion the GUI applies after a LaCAM run
        const auto pathsFile = std::filesystem::temp_directory_path() /
                               ("openmapf-plan-" + std::to_string(::getpid()) + ".txt");
        SolverOutcome outcome;
        LacamOutputParser("LaCAM").parse(m_startup.plan, pathsFile.string(), outcome);
        if (!outcome.errorMessage.empty() || outcome.pathsFile.empty()) {
            std::cerr << "Error loading plan '" << m_startup.plan << "': "
                      << (outcome.errorMessage.empty() ? "no solution in the file"
                                                       : outcome.errorMessage)
                      << std::endl;
            return false;
        }
        m_pathManager.loadPaths(outcome.pathsFile, true);
        std::filesystem::remove(outcome.pathsFile);
        m_animController.start(m_pathManager.getPaths());
        m_animController.setAnimating(true);
    }
    return true;
}

void ImGuiMapfApp::setupImGui(GLFWwindow* window, const char* glslVersion)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    m_uiManager.setupStyle();

    // Rasterize the font at the scaled size (sharper than FontGlobalScale)
    // and scale paddings, spacings and roundings to match.
    const float scale = m_env.m_ui_scale;
    if (scale != 1.0f) {
        ImFontConfig fontConfig;
        fontConfig.SizePixels = 13.0f * scale;
        io.Fonts->AddFontDefault(&fontConfig);
        ImGui::GetStyle().ScaleAllSizes(scale);

        ImPlotStyle& plot = ImPlot::GetStyle();
        for (ImVec2* v :
             {&plot.PlotDefaultSize, &plot.PlotMinSize, &plot.MajorTickLen, &plot.MinorTickLen,
              &plot.PlotPadding, &plot.LabelPadding, &plot.LegendPadding, &plot.LegendInnerPadding,
              &plot.LegendSpacing, &plot.MousePosPadding, &plot.AnnotationPadding}) {
            *v = ImVec2(v->x * scale, v->y * scale);
        }
        plot.PlotBorderSize *= scale;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    m_uiManager.setupFileDialogs();
}

void ImGuiMapfApp::cleanupImGui(GLFWwindow* window)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void ImGuiMapfApp::processFrame(GLFWwindow* window)
{
    const auto workStart = std::chrono::steady_clock::now();
    ImGuiIO& io = ImGui::GetIO();

    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();
    if (!m_layoutChecked) {
        // First start (no saved imgui.ini layout): dock the Scene on the left and
        // the Control Panel on the right instead of leaving them stacked.
        m_layoutChecked = true;
        const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace);
        if (node == nullptr || !node->IsSplitNode()) {
            ImGui::DockBuilderRemoveNode(dockspace);
            ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);
            ImGuiID left = 0;
            ImGuiID right = 0;
            ImGui::DockBuilderSplitNode(dockspace, ImGuiDir_Right, 0.3f, &right, &left);
            ImGui::DockBuilderDockWindow("Control Panel", right);
            ImGui::DockBuilderDockWindow("Scene", left);
            ImGui::DockBuilderFinish(dockspace);
        }
    }

    // Apply solver results queued by solver threads (GUI thread only)
    m_uiManager.processPendingResults();

    // Render scene window, control panel, file dialogs and animation
    renderScene();
    m_uiManager.renderControlPanel();
    m_uiManager.handleFileDialogs();
    m_animController.update(io.DeltaTime);

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    m_lastWorkMs =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - workStart)
            .count();
    glfwSwapBuffers(window);
}

void ImGuiMapfApp::renderScene()
{
    ImGui::Begin("Scene");

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 50.0f)
        canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f)
        canvas_size.y = 50.0f;

    ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw canvas background
    draw_list->AddRectFilled(canvas_pos, canvas_end, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));

    // Create invisible button for interaction
    ImGui::InvisibleButton("canvas", canvas_size,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool is_hovered = ImGui::IsItemHovered();

    // Handle viewport interactions (pan/zoom)
    ImGuiIO& io = ImGui::GetIO();
    m_viewportManager.handleInput(io, is_hovered, canvas_pos);
    const bool canvasResized = std::abs(canvas_size.x - m_lastCanvasSize.x) > 1.0f ||
                               std::abs(canvas_size.y - m_lastCanvasSize.y) > 1.0f;
    m_lastCanvasSize = canvas_size;
    if (m_uiManager.consumeFitViewRequest() || canvasResized) {
        // Jump when the Scene is (re)sized, e.g. while the docking layout
        // settles on startup; animate when the grid itself changed.
        fitViewToGrid(canvas_size, canvasResized);
    }
    m_viewportManager.update(io.DeltaTime);

    // Get current origin and zoom
    ImVec2 origin = m_viewportManager.getOrigin(canvas_pos);
    float zoom = m_viewportManager.getZoom();

    // Handle grid interactions
    if (m_uiManager.isAddRemoveObstacles()) {
        m_gridInteractionHandler.handleInteraction(ImGui::GetMousePos(), origin, zoom, is_hovered,
                                                   ImGui::IsMouseClicked(ImGuiMouseButton_Left),
                                                   m_uiManager.getDynamicAgents());
    } else if (m_uiManager.isAddRemoveAgents()) {
        m_uiManager.handleAgentInteraction(ImGui::GetMousePos(), origin, zoom, is_hovered,
                                           ImGui::IsMouseClicked(ImGuiMouseButton_Left));
    }

    // Render the scene content
    m_uiManager.renderSceneWindow(draw_list, canvas_pos, canvas_size, origin, zoom);

    ImGui::End();
}

void ImGuiMapfApp::fitViewToGrid(const ImVec2& canvasSize, bool immediate)
{
    const bool isHex = !m_env.m_square;
    const float tile = isHex ? m_env.m_hex_tile_size : m_env.m_square_tile_size;
    const HexGridType type = m_env.m_hex_grid_type;

    // Cell half-extents around the centers returned by gridToWorld: pointy-top
    // hexes (row layouts) are sqrt(3)*r wide and 2r tall, flat-top the reverse.
    ImVec2 half(tile * 0.5f, tile * 0.5f);
    if (isHex) {
        const bool pointy = type == HexGridType::ODD_R || type == HexGridType::EVEN_R;
        const float narrow = tile * 0.5f * std::sqrt(3.0f);
        half = pointy ? ImVec2(narrow, tile) : ImVec2(tile, narrow);
    }

    ImVec2 lo(FLT_MAX, FLT_MAX), hi(-FLT_MAX, -FLT_MAX);
    for (size_t y = 0; y < m_env.m_rows; ++y) {
        for (size_t x = 0; x < m_env.m_cols; ++x) {
            const ImVec2 c =
                GridRenderer::gridToWorld(x, y, isHex, tile, ImVec2(0.0f, 0.0f), 1.0f, type);
            lo = ImVec2(std::min(lo.x, c.x - half.x), std::min(lo.y, c.y - half.y));
            hi = ImVec2(std::max(hi.x, c.x + half.x), std::max(hi.y, c.y + half.y));
        }
    }
    if (lo.x <= hi.x) {
        m_viewportManager.fitTo(canvasSize, lo, hi, immediate);
    }
}

int ImGuiMapfApp::run()
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW error " << error << ": " << (description ? description : "?")
                  << std::endl;
    });

    if (!glfwInit()) {
        std::cerr << "Error: failed to initialize GLFW (is a display available?)" << std::endl;
        return 1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Honor configured window size, guarding against degenerate values
    int width = m_env.m_width >= 320 ? static_cast<int>(m_env.m_width) : 1280;
    int height = m_env.m_height >= 240 ? static_cast<int>(m_env.m_height) : 720;

    GLFWwindow* window =
        glfwCreateWindow(width, height, "OpenMAPF v" OPENMAPF_VERSION, nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Error: failed to create a window/OpenGL context" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    // OPENMAPF_VSYNC=0 renders as fast as possible (for performance measurements)
    const char* vsync = std::getenv("OPENMAPF_VSYNC");
    glfwSwapInterval(vsync != nullptr && std::strcmp(vsync, "0") == 0 ? 0 : 1);

    setupImGui(window, glsl_version);

    // OPENMAPF_FRAME_LOG=<file.csv>: one row per frame, for measuring GUI performance
    std::ofstream frameLog;
    if (const char* logPath = std::getenv("OPENMAPF_FRAME_LOG");
        logPath != nullptr && *logPath != '\0') {
        frameLog.open(logPath);
        if (!frameLog) {
            std::cerr << "Warning: cannot write the frame log '" << logPath << "'" << std::endl;
        } else {
            frameLog << "t_s,frame_ms,work_ms,rows,cols,grid,agents,animating,time_step\n";
            const auto* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            std::cerr << "Frame log: " << logPath
                      << " (OpenGL renderer: " << (renderer ? renderer : "unknown") << ")"
                      << std::endl;
        }
    }
    // The first logged frame also includes opening the startup files
    const auto start = std::chrono::steady_clock::now();
    auto last = start;
    if (!openStartupFiles()) {
        cleanupImGui(window);
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processFrame(window);
        if (frameLog.is_open()) {
            const auto now = std::chrono::steady_clock::now();
            frameLog << std::chrono::duration<double>(now - start).count() << ','
                     << std::chrono::duration<double, std::milli>(now - last).count() << ','
                     << m_lastWorkMs << ',' << m_env.m_rows << ',' << m_env.m_cols << ','
                     << gridName(m_env) << ',' << m_pathManager.getPaths().size() << ','
                     << (m_animController.isAnimating() ? 1 : 0) << ','
                     << m_animController.getCurrentTimeStep() << '\n'
                     << std::flush; // keep every row if the process is stopped by a signal
            last = now;
        }
    }

    cleanupImGui(window);
    return 0;
}