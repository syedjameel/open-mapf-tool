#include "Environment.h"
#include "MapFormat.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

Environment::Environment(const std::string& path)
{
    setDefaults();
    if (!path.empty()) {
        loadConfig(path);
    }
}

Environment::Environment()
{
    setDefaults();
}

Environment::~Environment() {}

void Environment::setDefaults()
{
    m_width = 1280;
    m_height = 720;
    m_square = true;
    m_hexagon = false;
    m_rows = 10;
    m_cols = 10;
    calculateTileSizes();

    // initialize a default map
    m_grid = Grid<int>(m_cols, m_rows, 0);
}

bool Environment::loadConfig(const std::string& config_path)
{
    // Never abort the application over a bad/missing config: keep the
    // compiled-in defaults, warn, and continue.
    try {
        YAML::Node config = YAML::LoadFile(config_path);

        if (!config["params"]) {
            std::cerr << "Warning: config file '" << config_path
                      << "' has no 'params' section; using built-in defaults." << std::endl;
            return false;
        }

        const YAML::Node& p = config["params"];
        m_width = p["width"].as<size_t>(m_width);
        m_height = p["height"].as<size_t>(m_height);
        m_square = p["square"].as<bool>(m_square);
        m_hexagon = p["hexagon"].as<bool>(m_hexagon);
        m_rows = p["rows"].as<size_t>(m_rows);
        m_cols = p["cols"].as<size_t>(m_cols);

        // Optional solver executable paths (absent => solver unavailable,
        // reported clearly when the user tries to run it).
        if (config["solvers"]) {
            const YAML::Node& s = config["solvers"];
            m_solver_cbs = s["cbs"].as<std::string>(m_solver_cbs);
            m_solver_lacam = s["lacam"].as<std::string>(m_solver_lacam);
            m_solver_lacam2 = s["lacam2"].as<std::string>(m_solver_lacam2);
            m_solver_lacam3 = s["lacam3"].as<std::string>(m_solver_lacam3);
        }

        m_grid = Grid<int>(m_cols, m_rows,
                           0); // reset the grid size whenever you read new row and col size
        calculateTileSizes();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Warning: failed to load config '" << config_path << "': " << e.what()
                  << "; using built-in defaults." << std::endl;
        setDefaults();
        return false;
    }
}

void Environment::loadMap(const std::string& map_path)
{
    std::ifstream file(map_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open map file: " + map_path);
    }

    std::string line;
    grid_data.clear();

    // Validate header
    std::getline(file, line);
    if (line.empty() || line[0] != 't') {
        throw std::runtime_error("Invalid map file header");
    }

    // Read dimensions
    std::getline(file, line);
    std::istringstream row_ss(line);
    std::string temp;
    if (!(row_ss >> temp >> m_rows) || temp != "height") {
        throw std::runtime_error("Invalid rows specification");
    }

    std::getline(file, line);
    std::istringstream col_ss(line);
    if (!(col_ss >> temp >> m_cols) || temp != "width") {
        throw std::runtime_error("Invalid columns specification");
    }

    // Skip separator line
    std::getline(file, line);

    // Read grid data
    m_grid = Grid<int>(m_cols, m_rows, 0);
    grid_data.reserve(m_rows * m_cols);

    for (size_t y = 0; y < m_rows && std::getline(file, line); ++y) {
        // Tolerate CRLF line endings: '\r' must not enter the grid as terrain
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        grid_data.push_back(line);
        for (size_t x = 0; x < m_cols && x < line.size(); ++x) {
            m_grid.set(x, y, MapFormat::charToCell(line[x]));
        }
    }

    // Update derived dimensions
    // m_tile_size = (m_height - 100) / std::max(m_cols, m_rows);
    // m_width = m_tile_size * m_cols + m_tile_size;
    // m_height = m_tile_size * m_rows + m_tile_size;
    // m_agent_size = m_tile_size / (2 * std::sqrt(2));

    grid_loaded = true;
}

void Environment::calculateTileSizes()
{
    // Calculate square tile dimensions
    m_square_tile_size = (m_height - UI_MARGIN) / std::max(m_cols, m_rows);
    m_square_agent_size = m_square_tile_size / (2 * SQRT_2);

    // Calculate hexagonal tile dimensions
    // Hexagon area = (3√3 / 2) * s² where s is the side length
    float temp = (m_height - UI_MARGIN) / std::max(m_cols, m_rows);
    m_hex_tile_size = std::sqrt(2 * (temp * temp) / HEX_AREA_FACTOR);
    m_hex_agent_size = SQRT_3 * (m_hex_tile_size / 4);
}