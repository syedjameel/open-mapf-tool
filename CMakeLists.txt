cmake_minimum_required(VERSION 3.10)
project(openmapf)

set(CMAKE_CXX_STANDARD 17)

# Create ImGui library
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(yaml-cpp REQUIRED)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)


# Add component files
add_executable(openmapf
    src/Main.cpp
    src/App.cpp
    src/Environment.cpp
    src/GridRenderer.cpp
    src/PathManager.cpp
    src/AnimationController.cpp
    src/UIManager.cpp
    src/ViewportManager.cpp
    src/GridInteractionHandler.cpp
    src/CBSDriver.cpp
)

target_include_directories(openmapf PRIVATE
    src
)

target_link_libraries(openmapf
    imgui
    yaml-cpp
    pthread
    dl
)