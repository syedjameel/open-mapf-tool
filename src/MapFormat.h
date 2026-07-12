#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "Grid.hpp"
#include "Types.h"

// Single source of truth for MovingAI benchmark terrain semantics
// (https://movingai.com/benchmarks/formats.html). Used by the map loader and
// every map/scenario writer so terrain round-trips losslessly.
namespace MapFormat {

// Grid cell values. Values other than Free/Obstacle preserve the original
// MovingAI terrain code so that saving a loaded map re-emits the same
// characters instead of silently converting terrain to free space.
enum Cell : int {
    Free        = 0, // '.' or 'G' — passable terrain
    Obstacle    = 1, // '@' — out of bounds / user-painted obstacle
    Tree        = 2, // 'T' — trees (impassable)
    Swamp       = 3, // 'S' — swamp
    Water       = 4, // 'W' — water
    OutOfBounds = 5, // 'O' — out of bounds
};

// True if agents can never occupy the cell. All non-Free terrain is treated
// as blocked (conservative for MAPF: S and W are not plain passable terrain).
inline bool isBlocked(int cell) { return cell != Free; }

// MovingAI character -> cell value. Unknown characters are treated as
// Obstacle (blocked) rather than free space.
int charToCell(char c);

// Cell value -> MovingAI character. Inverse of charToCell for known values;
// unknown values are written as '@'.
char cellToChar(int cell);

// Write a MovingAI .map file: "type octile" header plus terrain rows.
// Cells outside the grid's actual extent are emitted as free space.
void writeMap(std::ostream& out, const Grid<int>& grid, std::size_t cols, std::size_t rows);

// Write a MovingAI .scen (version 1) file. Emits one line per agent that has
// a goal: bucket, map name, map width, map height, start x, start y,
// goal x, goal y, optimal length (placeholder 0).
void writeScen(std::ostream& out, const std::vector<Agent>& agents,
               const std::string& mapFilename, std::size_t cols, std::size_t rows);

} // namespace MapFormat
