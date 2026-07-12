cmake_minimum_required(VERSION 3.16)
project(openmapf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(OPENMAPF_BUILD_TESTS "Build unit tests (fetches Catch2 via FetchContent)" ON)

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
    src/ConfigLocator.cpp
    src/Environment.cpp
    src/MapFormat.cpp
    src/GridRenderer.cpp
    src/PathManager.cpp
    src/AnimationController.cpp
    src/UIManager.cpp
    src/ViewportManager.cpp
    src/GridInteractionHandler.cpp
    src/CBSDriver.cpp
    src/LacamDriver.cpp
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

target_compile_definitions(openmapf PRIVATE
    OPENMAPF_VERSION="${PROJECT_VERSION}"
)

# ---------------------------------------------------------------------------
# Unit tests (headless: no OpenGL/GLFW context required)
# ---------------------------------------------------------------------------
if(OPENMAPF_BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(openmapf_tests
        tests/test_config_locator.cpp
        tests/test_environment.cpp
        tests/test_map_format.cpp
        tests/test_scen_writer.cpp
        tests/test_solver_driver.cpp
        src/CBSDriver.cpp
        src/ConfigLocator.cpp
        src/Environment.cpp
        src/LacamDriver.cpp
        src/MapFormat.cpp
    )

    # vendor/imgui is needed for headers only (Environment.h -> GridRenderer.h
    # -> imgui.h); no ImGui symbols are linked into the test binary.
    target_include_directories(openmapf_tests PRIVATE
        src
        vendor/imgui
        vendor/imgui/backends
    )

    target_link_libraries(openmapf_tests PRIVATE
        yaml-cpp
        Catch2::Catch2WithMain
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(openmapf_tests)
endif()