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

// ---------------------------------------------------------------------------
// MovingAI .scen (version 1) reading — accepts the standard benchmark
// scenario suites (Stern et al. 2019). Coordinates: x = column (0 = left),
// y = row (0 = top), matching the tool's internal (col, row) convention.
// ---------------------------------------------------------------------------

struct ScenEntry {
    int bucket = 0;
    std::string mapName;       // map file name the scenario was made for
    std::size_t mapWidth = 0;  // dimensions the scenario declares
    std::size_t mapHeight = 0;
    int startX = 0, startY = 0;
    int goalX = 0, goalY = 0;
    double optimalLength = 0.0;
};

struct ScenFile {
    std::vector<ScenEntry> entries;
    std::size_t malformedLines = 0; // skipped, not fatal
};

// Parse a .scen stream. The "version ..." header is optional (very old
// suites lack it); fields are whitespace-separated; CRLF is tolerated.
ScenFile readScen(std::istream& in);

// Convenience wrapper; throws std::runtime_error if the file cannot be read.
ScenFile loadScenFile(const std::string& path);

// Result of turning scen entries into placeable agents for a given grid.
struct ScenSelection {
    std::vector<Agent> agents;           // first maxAgents usable entries, ids 0..k-1
    std::size_t skippedOutOfBounds = 0;  // start/goal outside the grid
    std::size_t skippedBlocked = 0;      // start/goal on blocked terrain
    std::size_t skippedDuplicate = 0;    // start (or goal) cell already taken
    bool dimensionMismatch = false;      // scen declares different map dims
};

// Select the first `maxAgents` entries that fit the given grid, the way
// benchmark users expect ("run the first k agents of the scenario").
ScenSelection selectAgents(const std::vector<ScenEntry>& entries, const Grid<int>& grid,
                           std::size_t cols, std::size_t rows, std::size_t maxAgents);

} // namespace MapFormat
