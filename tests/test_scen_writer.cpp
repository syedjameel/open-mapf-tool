#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "MapFormat.h"
#include "Types.h"

TEST_CASE("writeScen emits MovingAI scen v1 with real map dimensions", "[scen]")
{
    std::vector<Agent> agents;
    agents.emplace_back(0, CoordUtils::make_coord(1, 2), CoordUtils::make_coord(3, 4));
    agents.emplace_back(1, CoordUtils::make_coord(5, 6)); // no goal: must be skipped
    agents.emplace_back(2, CoordUtils::make_coord(7, 8), CoordUtils::make_coord(9, 10));

    std::ostringstream out;
    // Regression test for defect A2: dimensions were hardcoded to 64 64.
    MapFormat::writeScen(out, agents, "arena.map", 12, 34);

    CHECK(out.str() == "version 1\n"
                       "0\tarena.map\t12\t34\t1\t2\t3\t4\t0\n"
                       "2\tarena.map\t12\t34\t7\t8\t9\t10\t0\n");
}

TEST_CASE("writeScen with no goal-carrying agents writes only the header", "[scen]")
{
    std::vector<Agent> agents;
    agents.emplace_back(0, CoordUtils::make_coord(1, 1)); // no goal

    std::ostringstream out;
    MapFormat::writeScen(out, agents, "empty.map", 8, 8);

    CHECK(out.str() == "version 1\n");
}
