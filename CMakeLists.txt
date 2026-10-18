cmake_minimum_required(VERSION 3.16)
project(openmapf VERSION 0.1.0 LANGUAGES CXX)

option(OPENMAPF_BUILD_TESTS "Build unit tests (fetches Catch2 via FetchContent)" ON)
option(OPENMAPF_WERROR "Treat first-party compiler warnings as errors (CI)" OFF)
option(OPENMAPF_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

if(OPENMAPF_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

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

option(OPENMAPF_FETCH_DEPS "Build yaml-cpp from source via FetchContent instead of requiring a system package" OFF)

find_package(Threads REQUIRED)

if(OPENMAPF_FETCH_DEPS)
    include(FetchContent)
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG        0.8.0
    )
    # yaml-cpp 0.8.0 declares cmake_minimum_required(3.4), which CMake >= 4.0
    # rejects; let it configure with 3.5 policies (ignored by older CMake).
    set(_openmapf_saved_policy_min "${CMAKE_POLICY_VERSION_MINIMUM}")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_MakeAvailable(yaml-cpp)
    set(CMAKE_POLICY_VERSION_MINIMUM "${_openmapf_saved_policy_min}")
    set(OPENMAPF_YAMLCPP_TARGET yaml-cpp::yaml-cpp)
else()
    find_package(yaml-cpp REQUIRED)
    # yaml-cpp exports 'yaml-cpp::yaml-cpp' from 0.8, plain 'yaml-cpp' before that
    if(TARGET yaml-cpp::yaml-cpp)
        set(OPENMAPF_YAMLCPP_TARGET yaml-cpp::yaml-cpp)
    else()
        set(OPENMAPF_YAMLCPP_TARGET yaml-cpp)
    endif()
endif()

# ---------------------------------------------------------------------------
# openmapf_core: headless static library (no ImGui, no OpenGL, no GLFW).
# Everything unit-testable and CLI-reusable lives here.
# ---------------------------------------------------------------------------
add_library(openmapf_core STATIC
    src/CBSDriver.cpp
    src/ConfigLocator.cpp
    src/Environment.cpp
    src/ExperimentRunner.cpp
    src/ExperimentSpec.cpp
    src/ExternalSolverDriver.cpp
    src/LacamDriver.cpp
    src/MapFormat.cpp
    src/PathManager.cpp
    src/ProcessRunner.cpp
    src/ResultsCsv.cpp
    src/ResultsSummary.cpp
    src/SolutionParsers.cpp
    src/SolverManifest.cpp
)

# Generated version header (OPENMAPF_VERSION), available to all targets
configure_file(src/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h @ONLY)

target_include_directories(openmapf_core PUBLIC
    src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
# vendored single-header subprocess library (implementation detail of
# ProcessRunner.cpp only; not exposed in public headers)
target_include_directories(openmapf_core PRIVATE vendor/subprocess)
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
    vendor/implot/implot.cpp
    vendor/implot/implot_items.cpp
)

target_include_directories(imgui PUBLIC
    vendor/imgui
    vendor/imgui/backends
    vendor/implot
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# 32-bit draw indices: a large map or many agents put more than 65,535
# vertices into one window's draw list, which 16-bit indices cannot address
# with the OpenGL 3.0 context we create (no ImGuiBackendFlags_RendererHasVtxOffset).
target_compile_definitions(imgui PUBLIC "ImDrawIdx=unsigned int")

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

# vendored imgui-filebrowser (header-only, GUI target only)
target_include_directories(openmapf PRIVATE vendor/imgui-filebrowser)

target_link_libraries(openmapf PRIVATE
    openmapf_core
    imgui
    ${CMAKE_DL_LIBS}
)

target_compile_options(openmapf PRIVATE ${OPENMAPF_WARNING_FLAGS})

# ---------------------------------------------------------------------------
# Installation & packaging
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS openmapf
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Installed next to bin/ so the exe-relative config discovery
# (<exe dir>/../share/openmapf/env.yaml) finds it out of the box.
install(FILES config/env.yaml config/solvers.yaml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/openmapf
)

install(FILES LICENSE README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

set(CPACK_PACKAGE_NAME "openmapf")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Interactive Multi-Agent Path Finding visualizer for square and hexagonal grids")
set(CPACK_PACKAGE_CONTACT "Jameel Ahmed Syed <syedjameel9290@gmail.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ;DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
include(CPack)

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
        tests/test_experiment_runner.cpp
        tests/test_experiment_spec.cpp
        tests/test_map_format.cpp
        tests/test_path_manager.cpp
        tests/test_process_runner.cpp
        tests/test_results_csv.cpp
        tests/test_results_summary.cpp
        tests/test_scen_reader.cpp
        tests/test_scen_writer.cpp
        tests/test_solver_driver.cpp
        tests/test_solver_integration.cpp
        tests/test_solver_output_parsers.cpp
        tests/test_solver_manifest.cpp
    )

    target_link_libraries(openmapf_tests PRIVATE
        openmapf_core
        Catch2::Catch2WithMain
    )

    target_compile_definitions(openmapf_tests PRIVATE
        OPENMAPF_TEST_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(openmapf_tests)
endif()
