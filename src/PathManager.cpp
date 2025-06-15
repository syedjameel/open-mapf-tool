#include "PathManager.h"
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <iostream>

void PathManager::loadPaths(const std::string& path, bool lacamFormat)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open paths file: " + path);
    }

    m_agentGridPaths.clear();
    std::string line;

    while (std::getline(file, line))
    {
        line.erase(line.find_last_not_of(" \n\r\t") + 1);
        if (line.empty())
            continue;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;

        std::string pathStr = line.substr(colonPos + 2);
        Path agentPath;
        size_t pos = 0;

        while (pos < pathStr.length())
        {
            size_t endPos = pathStr.find("->", pos);
            if (endPos == std::string::npos)
                endPos = pathStr.length();

            std::string coord = pathStr.substr(pos, endPos - pos);
            coord.erase(std::remove(coord.begin(), coord.end(), '('), coord.end());
            coord.erase(std::remove(coord.begin(), coord.end(), ')'), coord.end());

            size_t commaPos = coord.find(',');
            if (commaPos != std::string::npos)
            {
                try
                {
                    int x = std::stoi(coord.substr(0, commaPos));
                    int y = std::stoi(coord.substr(commaPos + 1));

                    if (!lacamFormat)
                    {
                        std::swap(x, y);
                    }
                    agentPath.emplace_back(CoordUtils::make_coord(x, y));
                }
                catch (const std::invalid_argument& e)
                {
                    std::cerr << "Warning: Invalid coordinate format in path file: " << coord << std::endl;
                    break;
                }
                catch (const std::out_of_range& e)
                {
                    std::cerr << "Warning: Coordinate value out of range in path file: " << coord << std::endl;
                    break;
                }
            }
            pos = endPos + 2;
        }

        if (!agentPath.empty())
        {
            m_agentGridPaths.push_back(agentPath);
        }
    }
}

void PathManager::clear()
{
    m_agentGridPaths.clear();
}