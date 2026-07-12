cmake_minimum_required(VERSION 3.16)
project(openmapf VERSION 0.1.0 LANGUAGES CXX)

option(OPENMAPF_BUILD_TESTS "Build unit tests (fetches Catch2 via FetchContent)" ON)
option(OPENMAPF_WERROR "Treat first-party compiler warnings as errors (CI)" OFF)

# Warning flags for first-party targets only (never for vendored code)
if(MSVC)
    set(OPENMAPF_WARNING_FLAGS /W4)
    if(OPENMAPF_WERROR)
        list(APPEND OPENMAPF_WARNING_FLAGS /WX)
    endif()
else()
    set(OPENMAPF_WARNING_FLAGS -Wall -Wextra)
    if(OPENMAPF_WERROR)
        list(APPEND OPENMAPF_WARNING_FLAGS -Werror)
    endif()
endif()

find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)

# yaml-cpp exports 'yaml-cpp::yaml-cpp' from 0.8, plain 'yaml-cpp' before that
if(TARGET yaml-cpp::yaml-cpp)
    set(OPENMAPF_YAMLCPP_TARGET yaml-cpp::yaml-cpp)
else()
    set(OPENMAPF_YAMLCPP_TARGET yaml-cpp)
endif()

# ---------------------------------------------------------------------------
# openmapf_core: headless static library (no ImGui, no OpenGL, no GLFW).
# Everything unit-testable and CLI-reusable lives here.
# ---------------------------------------------------------------------------
add_library(openmapf_core STATIC
    src/CBSDriver.cpp
    src/ConfigLocator.cpp
    src/Environment.cpp
    src/LacamDriver.cpp
    src/MapFormat.cpp
    src/PathManager.cpp
)

target_include_directories(openmapf_core PUBLIC src)
target_compile_features(openmapf_core PUBLIC cxx_std_17)
target_compile_options(openmapf_core PRIVATE ${OPENMAPF_WARNING_FLAGS})
target_link_libraries(openmapf_core PUBLIC
    ${OPENMAPF_YAMLCPP_TARGET}
    Threads::Threads
)

# ---------------------------------------------------------------------------
# Dear ImGui (vendored submodule) — GUI only
# ---------------------------------------------------------------------------
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# ---------------------------------------------------------------------------
# openmapf: the GUI executable
# ---------------------------------------------------------------------------
add_executable(openmapf
    src/Main.cpp
    src/App.cpp
    src/GridRenderer.cpp
    src/AnimationController.cpp
    src/UIManager.cpp
    src/ViewportManager.cpp
    src/GridInteractionHandler.cpp
)

target_link_libraries(openmapf PRIVATE
    openmapf_core
    imgui
    ${CMAKE_DL_LIBS}
)

target_compile_options(openmapf PRIVATE ${OPENMAPF_WARNING_FLAGS})

target_compile_definitions(openmapf PRIVATE
    OPENMAPF_VERSION="${PROJECT_VERSION}"
)

# ---------------------------------------------------------------------------
# Unit tests (headless: link openmapf_core only — no ImGui/OpenGL/GLFW)
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
    )

    target_link_libraries(openmapf_tests PRIVATE
        openmapf_core
        Catch2::Catch2WithMain
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(openmapf_tests)
endif()
