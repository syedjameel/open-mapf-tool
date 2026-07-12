#include <catch2/catch_test_macros.hpp>

#include "Environment.h"
#include "TestHelpers.h"

TEST_CASE("loadMap parses a valid MovingAI map", "[environment]")
{
    TempFile map(
        "type octile\n"
        "height 3\n"
        "width 4\n"
        "map\n"
        "....\n"
        ".@@.\n"
        "....\n");

    Environment env;
    env.loadMap(map.path());

    CHECK(env.m_rows == 3);
    CHECK(env.m_cols == 4);
    CHECK(env.grid_loaded);
    CHECK(env.m_grid.get(0, 0) == 0);
    CHECK(env.m_grid.get(1, 1) == 1);
    CHECK(env.m_grid.get(2, 1) == 1);
    CHECK(env.m_grid.get(3, 2) == 0);
}

TEST_CASE("loadMap rejects bad input", "[environment]")
{
    Environment env;

    SECTION("missing file")
    {
        CHECK_THROWS(env.loadMap("/nonexistent/path/to/file.map"));
    }

    SECTION("bad header")
    {
        TempFile map("not-a-map\nheight 2\nwidth 2\nmap\n..\n..\n");
        CHECK_THROWS(env.loadMap(map.path()));
    }

    SECTION("bad dimension line")
    {
        TempFile map("type octile\nrows 2\nwidth 2\nmap\n..\n..\n");
        CHECK_THROWS(env.loadMap(map.path()));
    }
}
