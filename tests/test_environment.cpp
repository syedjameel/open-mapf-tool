#include <catch2/catch_test_macros.hpp>

#include "Environment.h"
#include "TestHelpers.h"

TEST_CASE("loadMap parses a valid MovingAI map", "[environment]")
{
    TempFile map("type octile\n"
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

    // Tile sizes are recomputed for the loaded map (4 columns > 3 rows).
    CHECK(env.m_square_tile_size == (env.m_height - UI_MARGIN) / 4.0f);
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

TEST_CASE("loadConfig reads and clamps ui_scale", "[environment]")
{
    SECTION("absent key keeps the default")
    {
        TempFile cfg("params:\n  rows: 5\n");
        Environment env(cfg.path());
        CHECK(env.m_ui_scale == 1.0f);
    }

    SECTION("valid value")
    {
        TempFile cfg("params:\n  ui_scale: 2.0\n");
        Environment env(cfg.path());
        CHECK(env.m_ui_scale == 2.0f);
    }

    SECTION("out-of-range values are clamped")
    {
        TempFile big("params:\n  ui_scale: 9\n");
        CHECK(Environment(big.path()).m_ui_scale == Environment::UI_SCALE_MAX);
        TempFile small("params:\n  ui_scale: 0.1\n");
        CHECK(Environment(small.path()).m_ui_scale == Environment::UI_SCALE_MIN);
    }

    SECTION("non-numeric value falls back to defaults")
    {
        TempFile cfg("params:\n  ui_scale: huge\n");
        Environment env(cfg.path());
        CHECK(env.m_ui_scale == 1.0f);
    }
}
