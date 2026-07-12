#include "MapFormat.h"

#include <ostream>

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

} // namespace MapFormat
