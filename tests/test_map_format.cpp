#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "Environment.h"
#include "MapFormat.h"
#include "TestHelpers.h"

TEST_CASE("MovingAI terrain codes map to blocked/free cells", "[mapformat]")
{
    // Passable terrain
    CHECK(MapFormat::charToCell('.') == MapFormat::Free);
    CHECK(MapFormat::charToCell('G') == MapFormat::Free);
    CHECK_FALSE(MapFormat::isBlocked(MapFormat::charToCell('.')));
    CHECK_FALSE(MapFormat::isBlocked(MapFormat::charToCell('G')));

    // Blocked terrain per MovingAI spec (conservative for MAPF)
    for (char c : {'@', 'O', 'T', 'S', 'W'}) {
        INFO("terrain code: " << c);
        CHECK(MapFormat::isBlocked(MapFormat::charToCell(c)));
    }

    // Unknown characters must never become free space
    CHECK(MapFormat::isBlocked(MapFormat::charToCell('?')));
    CHECK(MapFormat::isBlocked(MapFormat::charToCell('\r')));
}

TEST_CASE("charToCell and cellToChar are inverse for known terrain", "[mapformat]")
{
    for (char c : {'.', '@', 'T', 'S', 'W', 'O'}) {
        INFO("terrain code: " << c);
        CHECK(MapFormat::cellToChar(MapFormat::charToCell(c)) == c);
    }
    // 'G' is passable and normalizes to '.'
    CHECK(MapFormat::cellToChar(MapFormat::charToCell('G')) == '.');
    // Unknown cell values are written as blocked, never as free
    CHECK(MapFormat::cellToChar(999) == '@');
}

TEST_CASE("map load/write round-trips all MovingAI terrain codes", "[mapformat]")
{
    const std::string terrainRows = ".@T.\n"
                                    "SWO.\n"
                                    "T..@\n";
    TempFile map("type octile\n"
                 "height 3\n"
                 "width 4\n"
                 "map\n" +
                 terrainRows);

    Environment env;
    env.loadMap(map.path());

    // T (trees) must load as blocked -- regression test for defect A1 where
    // T loaded as a pseudo-value that every writer re-emitted as free space.
    CHECK(MapFormat::isBlocked(env.m_grid.get(2, 0)));       // T
    CHECK(MapFormat::isBlocked(env.m_grid.get(0, 1)));       // S
    CHECK(MapFormat::isBlocked(env.m_grid.get(1, 1)));       // W
    CHECK(MapFormat::isBlocked(env.m_grid.get(2, 1)));       // O
    CHECK_FALSE(MapFormat::isBlocked(env.m_grid.get(0, 0))); // .

    std::ostringstream out;
    MapFormat::writeMap(out, env.m_grid, env.m_cols, env.m_rows);

    CHECK(out.str() == "type octile\n"
                       "height 3\n"
                       "width 4\n"
                       "map\n" +
                           terrainRows);
}

TEST_CASE("map loader tolerates CRLF line endings", "[mapformat]")
{
    TempFile map("type octile\r\n"
                 "height 2\r\n"
                 "width 2\r\n"
                 "map\r\n"
                 ".@\r\n"
                 "T.\r\n");

    Environment env;
    env.loadMap(map.path());

    CHECK(env.m_rows == 2);
    CHECK(env.m_cols == 2);
    CHECK(env.m_grid.get(0, 0) == MapFormat::Free);
    CHECK(env.m_grid.get(1, 0) == MapFormat::Obstacle);
    CHECK(env.m_grid.get(0, 1) == MapFormat::Tree);
    CHECK(env.m_grid.get(1, 1) == MapFormat::Free);
}

TEST_CASE("writeMap pads cells outside the grid extent as free", "[mapformat]")
{
    Grid<int> grid(2, 2, 0);
    grid.set(0, 0, MapFormat::Obstacle);

    std::ostringstream out;
    MapFormat::writeMap(out, grid, 3, 2); // one column wider than the grid

    CHECK(out.str() == "type octile\n"
                       "height 2\n"
                       "width 3\n"
                       "map\n"
                       "@..\n"
                       "...\n");
}
