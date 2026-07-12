#include <catch2/catch_test_macros.hpp>

#include "PathManager.h"
#include "TestHelpers.h"

TEST_CASE("loadPaths parses arrow files with the CBS (row,col) convention", "[paths]")
{
    // CBS arrow files store (row,col); loading with lacamFormat=false must
    // swap into the internal (x=col, y=row) convention. Regression guard for
    // the auto-load bug where the swap depended on a UI checkbox.
    TempFile file(
        "Agent 0: (1,2)->(1,3)->(2,3)\n"
        "Agent 1: (5,0)->(4,0)\n",
        ".txt");

    PathManager pm;
    pm.loadPaths(file.path(), /*lacamFormat=*/false);

    const auto& paths = pm.getPaths();
    REQUIRE(paths.size() == 2);
    REQUIRE(paths[0].size() == 3);
    // (row=1, col=2) -> internal (x=2, y=1)
    CHECK(CoordUtils::x(paths[0][0]) == 2);
    CHECK(CoordUtils::y(paths[0][0]) == 1);
    CHECK(CoordUtils::x(paths[0][2]) == 3);
    CHECK(CoordUtils::y(paths[0][2]) == 2);
    CHECK(CoordUtils::x(paths[1][1]) == 0);
    CHECK(CoordUtils::y(paths[1][1]) == 4);
}

TEST_CASE("loadPaths keeps (x,y) order for LaCAM-converted files", "[paths]")
{
    TempFile file("Agent 0: (1,2)->(1,3)\n", ".txt");

    PathManager pm;
    pm.loadPaths(file.path(), /*lacamFormat=*/true);

    const auto& paths = pm.getPaths();
    REQUIRE(paths.size() == 1);
    CHECK(CoordUtils::x(paths[0][0]) == 1);
    CHECK(CoordUtils::y(paths[0][0]) == 2);
}

TEST_CASE("loadPaths edge cases", "[paths]")
{
    SECTION("empty lines and lines without ':' are skipped")
    {
        TempFile file("\nnot a path line\nAgent 0: (0,0)->(0,1)\n\n", ".txt");
        PathManager pm;
        pm.loadPaths(file.path(), true);
        CHECK(pm.getPaths().size() == 1);
    }

    SECTION("malformed coordinate stops that agent's path at the last valid step")
    {
        TempFile file("Agent 0: (0,0)->(zz,1)->(2,2)\nAgent 1: (3,3)\n", ".txt");
        PathManager pm;
        pm.loadPaths(file.path(), true);
        REQUIRE(pm.getPaths().size() == 2);
        CHECK(pm.getPaths()[0].size() == 1); // only (0,0) survived
        CHECK(pm.getPaths()[1].size() == 1);
    }

    SECTION("missing trailing newline is fine")
    {
        TempFile file("Agent 0: (0,0)->(0,1)", ".txt");
        PathManager pm;
        pm.loadPaths(file.path(), true);
        REQUIRE(pm.getPaths().size() == 1);
        CHECK(pm.getPaths()[0].size() == 2);
    }

    SECTION("missing file throws")
    {
        PathManager pm;
        CHECK_THROWS(pm.loadPaths("/nonexistent/paths.txt", false));
    }
}
