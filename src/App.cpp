#include "App.h"
#include <iostream>

void ImGuiMapfApp::setupImGui(GLFWwindow* window, const char* glslVersion)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    m_uiManager.setupStyle();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    
    m_uiManager.setupFileDialogs();
}

void ImGuiMapfApp::cleanupImGui(GLFWwindow* window)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void ImGuiMapfApp::processFrame(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();
    
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
    glfwSwapBuffers(window);
}

void ImGuiMapfApp::renderScene()
{
    ImGui::Begin("Scene");
    
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
    
    ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw canvas background
    draw_list->AddRectFilled(canvas_pos, canvas_end, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_pos, canvas_end, IM_COL32(255, 255, 255, 255));
    
    // Create invisible button for interaction
    ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool is_hovered = ImGui::IsItemHovered();
    
    // Handle viewport interactions (pan/zoom)
    ImGuiIO& io = ImGui::GetIO();
    m_viewportManager.handleInput(io, is_hovered, canvas_pos);
    m_viewportManager.update(io.DeltaTime);
    
    // Get current origin and zoom
    ImVec2 origin = m_viewportManager.getOrigin(canvas_pos);
    float zoom = m_viewportManager.getZoom();
    
    // Handle grid interactions
    if (m_uiManager.isAddRemoveObstacles())
    {
        m_gridInteractionHandler.handleInteraction(
            ImGui::GetMousePos(), origin, zoom, 
            is_hovered, ImGui::IsMouseClicked(ImGuiMouseButton_Left),
            m_uiManager.getDynamicAgents()
        );
    }
    else if (m_uiManager.isAddRemoveAgents())
    {
        m_uiManager.handleAgentInteraction(
            ImGui::GetMousePos(), origin, zoom,
            is_hovered, ImGui::IsMouseClicked(ImGuiMouseButton_Left)
        );
    }
    
    // Render the scene content
    m_uiManager.renderSceneWindow(draw_list, canvas_pos, canvas_size, origin, zoom);
    
    ImGui::End();
}

int ImGuiMapfApp::run()
{
    glfwSetErrorCallback([](int error, const char* description) {
        std::cerr << "GLFW error " << error << ": " << (description ? description : "?") << std::endl;
    });

    if (!glfwInit())
    {
        std::cerr << "Error: failed to initialize GLFW (is a display available?)" << std::endl;
        return 1;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Honor configured window size, guarding against degenerate values
    int width = m_env.m_width >= 320 ? static_cast<int>(m_env.m_width) : 1280;
    int height = m_env.m_height >= 240 ? static_cast<int>(m_env.m_height) : 720;

    GLFWwindow* window = glfwCreateWindow(width, height, "OpenMAPF", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Error: failed to create a window/OpenGL context" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    setupImGui(window, glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        processFrame(window);
    }

    cleanupImGui(window);
    return 0;
}