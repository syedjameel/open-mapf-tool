#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <sstream>

#include "MapFormat.h"
#include "TestHelpers.h"

#ifndef OPENMAPF_TEST_FIXTURES
#error "OPENMAPF_TEST_FIXTURES must be defined by the build"
#endif

namespace {
const std::string kFixtures = OPENMAPF_TEST_FIXTURES;
}

TEST_CASE("readScen parses an authentic Stern benchmark scenario", "[scen]")
{
    // tests/fixtures/empty-8-8-random-1.scen is the unmodified file from the
    // MovingAI MAPF benchmark suite (movingai.com/benchmarks/mapf).
    auto scen = MapFormat::loadScenFile(kFixtures + "/empty-8-8-random-1.scen");

    CHECK(scen.malformedLines == 0);
    REQUIRE(scen.entries.size() == 32);

    const auto& e = scen.entries.front();
    CHECK(e.bucket == 1);
    CHECK(e.mapName == "empty-8-8.map");
    CHECK(e.mapWidth == 8);
    CHECK(e.mapHeight == 8);
    CHECK(e.startX == 1);
    CHECK(e.startY == 4);
    CHECK(e.goalX == 4);
    CHECK(e.goalY == 7);
    CHECK_THAT(e.optimalLength, Catch::Matchers::WithinAbs(4.24264069, 1e-8));

    for (const auto& entry : scen.entries) {
        CHECK(entry.mapWidth == 8);
        CHECK(entry.startX >= 0);
        CHECK(entry.startX < 8);
        CHECK(entry.goalY < 8);
    }
}

TEST_CASE("readScen tolerates format variations", "[scen]")
{
    SECTION("missing version header")
    {
        std::istringstream in("0\tm.map\t8\t8\t1\t2\t3\t4\t2.0\n");
        auto scen = MapFormat::readScen(in);
        REQUIRE(scen.entries.size() == 1);
        CHECK(scen.entries[0].startX == 1);
    }

    SECTION("CRLF line endings and blank lines")
    {
        std::istringstream in("version 1\r\n\r\n0\tm.map\t8\t8\t1\t2\t3\t4\t2.0\r\n");
        auto scen = MapFormat::readScen(in);
        REQUIRE(scen.entries.size() == 1);
        CHECK(scen.entries[0].goalY == 4);
    }

    SECTION("space-separated fields")
    {
        std::istringstream in("version 1\n0 m.map 8 8 1 2 3 4 2.0\n");
        auto scen = MapFormat::readScen(in);
        REQUIRE(scen.entries.size() == 1);
    }

    SECTION("malformed lines are skipped and counted")
    {
        std::istringstream in(
            "version 1\n"
            "0\tm.map\t8\t8\t1\t2\t3\t4\t2.0\n"
            "not a scen line\n"
            "0\tm.map\t8\t8\tX\tY\t3\t4\t2.0\n"
            "0\tm.map\t8\t8\t5\t6\t7\t0\t2.0\n");
        auto scen = MapFormat::readScen(in);
        CHECK(scen.entries.size() == 2);
        CHECK(scen.malformedLines == 2);
    }

    SECTION("empty file")
    {
        std::istringstream in("");
        auto scen = MapFormat::readScen(in);
        CHECK(scen.entries.empty());
        CHECK(scen.malformedLines == 0);
    }

    SECTION("unreadable path throws")
    {
        CHECK_THROWS(MapFormat::loadScenFile("/nonexistent/foo.scen"));
    }
}

TEST_CASE("writeScen output round-trips through readScen", "[scen]")
{
    std::vector<Agent> agents;
    agents.emplace_back(0, CoordUtils::make_coord(1, 2), CoordUtils::make_coord(3, 4));
    agents.emplace_back(1, CoordUtils::make_coord(5, 6), CoordUtils::make_coord(7, 0));

    std::ostringstream out;
    MapFormat::writeScen(out, agents, "arena.map", 12, 34);

    std::istringstream in(out.str());
    auto scen = MapFormat::readScen(in);

    REQUIRE(scen.entries.size() == 2);
    CHECK(scen.malformedLines == 0);
    CHECK(scen.entries[0].mapName == "arena.map");
    CHECK(scen.entries[0].mapWidth == 12);
    CHECK(scen.entries[0].mapHeight == 34);
    CHECK(scen.entries[0].startX == 1);
    CHECK(scen.entries[0].startY == 2);
    CHECK(scen.entries[0].goalX == 3);
    CHECK(scen.entries[0].goalY == 4);
    CHECK(scen.entries[1].startX == 5);
    CHECK(scen.entries[1].goalY == 0);
}

TEST_CASE("selectAgents takes the first k usable entries", "[scen]")
{
    auto scen = MapFormat::loadScenFile(kFixtures + "/empty-8-8-random-1.scen");
    Grid<int> grid(8, 8, MapFormat::Free);

    SECTION("first k agents, ids reassigned 0..k-1")
    {
        auto sel = MapFormat::selectAgents(scen.entries, grid, 8, 8, 5);
        REQUIRE(sel.agents.size() == 5);
        for (std::size_t i = 0; i < sel.agents.size(); ++i) {
            CHECK(sel.agents[i].id == i);
            CHECK(sel.agents[i].hasGoal);
        }
        // First agent matches the first scen line
        CHECK(CoordUtils::x(sel.agents[0].start) == 1);
        CHECK(CoordUtils::y(sel.agents[0].start) == 4);
        CHECK(CoordUtils::x(sel.agents[0].goal) == 4);
        CHECK(CoordUtils::y(sel.agents[0].goal) == 7);
        CHECK_FALSE(sel.dimensionMismatch);
    }

    SECTION("asking for more agents than the scen has yields all usable ones")
    {
        auto sel = MapFormat::selectAgents(scen.entries, grid, 8, 8, 1000);
        CHECK(sel.agents.size() + sel.skippedOutOfBounds + sel.skippedBlocked +
                  sel.skippedDuplicate ==
              scen.entries.size());
        CHECK(sel.agents.size() > 0);
    }
}

TEST_CASE("selectAgents skips unusable entries with per-reason counts", "[scen]")
{
    Grid<int> grid(4, 4, MapFormat::Free);
    grid.set(2, 2, MapFormat::Obstacle);
    grid.set(3, 3, MapFormat::Tree);

    auto entry = [](int sx, int sy, int gx, int gy, std::size_t w = 4, std::size_t h = 4) {
        MapFormat::ScenEntry e;
        e.mapName = "m.map";
        e.mapWidth = w;
        e.mapHeight = h;
        e.startX = sx; e.startY = sy;
        e.goalX = gx; e.goalY = gy;
        return e;
    };

    std::vector<MapFormat::ScenEntry> entries = {
        entry(0, 0, 1, 1),   // ok
        entry(9, 0, 1, 0),   // start out of bounds (scen for a bigger map)
        entry(0, 1, 2, 2),   // goal on obstacle
        entry(3, 3, 0, 2),   // start on tree terrain
        entry(0, 0, 1, 2),   // duplicate start cell
        entry(0, 2, 1, 1),   // duplicate goal cell
        entry(1, 2, 2, 1),   // ok
    };

    auto sel = MapFormat::selectAgents(entries, grid, 4, 4, 100);

    CHECK(sel.agents.size() == 2);
    CHECK(sel.skippedOutOfBounds == 1);
    CHECK(sel.skippedBlocked == 2);
    CHECK(sel.skippedDuplicate == 2);

    // Entries declaring different map dimensions raise the mismatch flag
    std::vector<MapFormat::ScenEntry> mismatched = {entry(0, 0, 1, 1, 64, 64)};
    auto sel2 = MapFormat::selectAgents(mismatched, grid, 4, 4, 100);
    CHECK(sel2.dimensionMismatch);
    CHECK(sel2.agents.size() == 1); // still usable if it fits
}
