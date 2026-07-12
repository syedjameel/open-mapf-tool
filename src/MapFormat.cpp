#include "MapFormat.h"

#include <fstream>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace MapFormat {

int charToCell(char c)
{
    switch (c) {
        case '.':
        case 'G':
            return Free;
        case '@':
            return Obstacle;
        case 'T':
            return Tree;
        case 'S':
            return Swamp;
        case 'W':
            return Water;
        case 'O':
            return OutOfBounds;
        default:
            return Obstacle; // unknown terrain: block it, never make it free
    }
}

char cellToChar(int cell)
{
    switch (cell) {
        case Free:
            return '.';
        case Obstacle:
            return '@';
        case Tree:
            return 'T';
        case Swamp:
            return 'S';
        case Water:
            return 'W';
        case OutOfBounds:
            return 'O';
        default:
            return '@'; // unknown value: block it, never make it free
    }
}

void writeMap(std::ostream& out, const Grid<int>& grid, std::size_t cols, std::size_t rows)
{
    out << "type octile\n";
    out << "height " << rows << "\n";
    out << "width " << cols << "\n";
    out << "map\n";

    for (std::size_t row = 0; row < rows; ++row) {
        for (std::size_t col = 0; col < cols; ++col) {
            if (col < grid.width() && row < grid.height()) {
                out << cellToChar(grid.get(col, row));
            } else {
                out << '.';
            }
        }
        out << "\n";
    }
}

void writeScen(std::ostream& out, const std::vector<Agent>& agents,
               const std::string& mapFilename, std::size_t cols, std::size_t rows)
{
    out << "version 1\n";

    for (const auto& agent : agents) {
        if (!agent.hasGoal) {
            continue;
        }
        out << agent.id << "\t" << mapFilename << "\t"
            << cols << "\t" << rows << "\t"
            << CoordUtils::x(agent.start) << "\t" << CoordUtils::y(agent.start) << "\t"
            << CoordUtils::x(agent.goal) << "\t" << CoordUtils::y(agent.goal) << "\t"
            << "0\n";
    }
}

ScenFile readScen(std::istream& in)
{
    ScenFile result;
    std::string line;
    bool first = true;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // tolerate CRLF
        }
        if (line.empty()) {
            continue;
        }
        // Optional "version ..." header on the first non-empty line
        if (first && line.rfind("version", 0) == 0) {
            first = false;
            continue;
        }
        first = false;

        std::istringstream ss(line);
        ScenEntry e;
        if (ss >> e.bucket >> e.mapName >> e.mapWidth >> e.mapHeight >> e.startX >> e.startY >>
            e.goalX >> e.goalY >> e.optimalLength) {
            result.entries.push_back(std::move(e));
        } else {
            ++result.malformedLines;
        }
    }
    return result;
}

ScenFile loadScenFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open scenario file: " + path);
    }
    return readScen(file);
}

ScenSelection selectAgents(const std::vector<ScenEntry>& entries, const Grid<int>& grid,
                           std::size_t cols, std::size_t rows, std::size_t maxAgents)
{
    ScenSelection sel;
    std::set<std::pair<int, int>> usedStarts;
    std::set<std::pair<int, int>> usedGoals;

    for (const ScenEntry& e : entries) {
        if (sel.agents.size() >= maxAgents) {
            break;
        }
        if (e.mapWidth != cols || e.mapHeight != rows) {
            sel.dimensionMismatch = true;
        }

        auto inBounds = [&](int x, int y) {
            return x >= 0 && y >= 0 && static_cast<std::size_t>(x) < cols &&
                   static_cast<std::size_t>(y) < rows;
        };
        if (!inBounds(e.startX, e.startY) || !inBounds(e.goalX, e.goalY)) {
            ++sel.skippedOutOfBounds;
            continue;
        }
        if (isBlocked(grid.get(e.startX, e.startY)) || isBlocked(grid.get(e.goalX, e.goalY))) {
            ++sel.skippedBlocked;
            continue;
        }
        if (usedStarts.count({e.startX, e.startY}) != 0 || usedGoals.count({e.goalX, e.goalY}) != 0) {
            ++sel.skippedDuplicate;
            continue;
        }

        usedStarts.insert({e.startX, e.startY});
        usedGoals.insert({e.goalX, e.goalY});
        sel.agents.emplace_back(sel.agents.size(), CoordUtils::make_coord(e.startX, e.startY),
                                CoordUtils::make_coord(e.goalX, e.goalY));
    }
    return sel;
}

} // namespace MapFormat
