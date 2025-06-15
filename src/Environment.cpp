#include "Environment.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

Environment::Environment(const std::string& path)
{
    loadConfig(path);
}

Environment::Environment()
{
    m_width = 1280;
    m_height = 720;
    m_name = "default";
    m_square = true;
    m_hexagon = false;
    m_rows = 10;
    m_cols = 10;
    m_frame_rate = 60;
    m_time_step = 1.f;
    calculateTileSizes();

    // initialize a default map
    m_grid = Grid<int>(m_cols, m_rows, 0);
}

Environment::~Environment()
{
}

void Environment::loadConfig(const std::string& config_path)
{
    YAML::Node config = YAML::LoadFile(config_path);

    if (config["params"]) {
        m_width = config["params"]["width"].as<size_t>();
        m_height = config["params"]["height"].as<size_t>();
        m_name = config["params"]["name"].as<std::string>();
        m_square = config["params"]["square"].as<bool>();
        m_hexagon = config["params"]["hexagon"].as<bool>();
        m_rows = config["params"]["rows"].as<size_t>();
        m_cols = config["params"]["cols"].as<size_t>();
        m_frame_rate = config["params"]["frame_rate"].as<size_t>();
        m_time_step = config["params"]["time_step"].as<float>();
        m_grid = Grid<int>(m_cols, m_rows, 0); // reset the grid size whenever you read new row and col size
        calculateTileSizes();
        }
    // } else {
    //     std::cerr << "Invalid configuration format." << std::endl;
    // }

    // loadMap();
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
        grid_data.push_back(line);
        for (size_t x = 0; x < m_cols && x < line.size(); ++x) {
            if (line[x] == '.') {
                m_grid.set(x, y, 0);
            } else if (line[x] == '@') {
                m_grid.set(x, y, 1);
            } else {
                m_grid.set(x, y, 2);
            }
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