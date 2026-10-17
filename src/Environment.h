#pragma once

#include "Grid.hpp"
#include "HexGridType.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// Constants for tile size calculations
static constexpr float UI_MARGIN = 100.0f;
static constexpr float SQRT_2 = 1.414213562f;
static constexpr float SQRT_3 = 1.732050808f;
static constexpr float HEX_AREA_FACTOR = 3.0f * SQRT_3 / 2.0f;

class Environment
{
public:
    size_t m_width{0};
    size_t m_height{0};
    bool m_square{false};
    bool m_hexagon{false};

    size_t m_rows{0};
    size_t m_cols{0};

    // GUI scale for fonts and widget sizes (HiDPI screens, screen recordings).
    float m_ui_scale{1.0f};
    static constexpr float UI_SCALE_MIN = 0.5f;
    static constexpr float UI_SCALE_MAX = 4.0f;

    float m_tile_size{10};

    float m_hex_tile_size{1};
    float m_square_tile_size{1};

    float m_agent_size{5};

    float m_hex_agent_size{5};
    float m_square_agent_size{5};

    // Hex grid type (odd-r, even-r, odd-q, even-q)
    HexGridType m_hex_grid_type{HexGridType::ODD_R};

    // Solver executables, configured via the optional 'solvers' section of
    // env.yaml. Empty string = solver not configured; the UI reports this
    // clearly instead of invoking a nonexistent binary.
    std::string m_solver_cbs;
    std::string m_solver_lacam;
    std::string m_solver_lacam2;
    std::string m_solver_lacam3;

    std::vector<std::string> grid_data;
    bool grid_loaded = false;

    Grid<int> m_grid;

    Environment();
    explicit Environment(const std::string& config_path);
    ~Environment();

    // Loads config from a YAML file. Returns false (keeping built-in
    // defaults) instead of throwing when the file is missing or invalid.
    bool loadConfig(const std::string& config_path);
    void loadMap(const std::string& map_path);

private:
    void setDefaults();
    void calculateTileSizes();
};