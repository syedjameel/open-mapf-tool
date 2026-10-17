#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <fstream>

#include "ResultsCsv.h"
#include "ResultsSummary.h"
#include "TestHelpers.h"

namespace {

std::vector<std::string> header()
{
    return ResultsCsv::splitRow(ResultsCsv::kHeader);
}

std::vector<std::string> row(const std::string& solver, int agents, const std::string& grid,
                             const std::string& layout, bool solved, double runtime, long cost,
                             long lb, const std::string& status)
{
    RunRecord r;
    r.solver = solver;
    r.map = "m";
    r.scenario = "s";
    r.agents = static_cast<std::size_t>(agents);
    r.gridType = grid;
    r.hexLayout = layout;
    r.solved = solved;
    r.runtimeS = runtime;
    r.solutionCost = cost;
    r.socLowerBound = lb;
    r.status = status;
    return ResultsCsv::splitRow(ResultsCsv::formatRow(r));
}

} // namespace

TEST_CASE("summary groups by solver x grid and agent count", "[summary]")
{
    const auto s = ResultsSummary::fromRows(
        header(), {row("lacam", 50, "square", "", true, 0.1, 100, 80, "solved"),
                   row("lacam", 50, "square", "", true, 0.3, 140, 100, "solved"),
                   row("lacam", 100, "square", "", false, 2.0, -1, -1, "unsolved"),
                   row("lacam", 50, "hex", "oddr", true, 0.2, 90, 90, "solved")});

    REQUIRE(s.series.size() == 2);
    CHECK(s.series[0].label == "lacam (hex-oddr)");
    CHECK(s.series[1].label == "lacam (square)");

    const auto& sq = s.series[1].points;
    REQUIRE(sq.size() == 2);
    CHECK(sq[0].agents == 50);
    CHECK(sq[0].runs == 2);
    CHECK(sq[0].solved == 2);
    CHECK(sq[0].meanCost == Catch::Approx(120.0));
    CHECK(sq[0].sdCost == Catch::Approx(std::sqrt(800.0)));
    CHECK(sq[0].meanRuntimeS == Catch::Approx(0.2));
    CHECK(sq[0].meanCostOverLb == Catch::Approx((1.25 + 1.4) / 2));
    CHECK(sq[1].agents == 100);
    CHECK(sq[1].successRate() == 0.0);
    CHECK(std::isnan(sq[1].meanCost));
    CHECK(s.hasLowerBounds);
}

TEST_CASE("summary leaves out skipped runs and tolerates old schemas", "[summary]")
{
    auto s = ResultsSummary::fromRows(
        header(), {row("lacam3", 500, "square", "", false, 0, -1, -1, "skipped"),
                   row("lacam3", 50, "square", "", true, 1.0, 100, -1, "solved")});
    REQUIRE(s.series.size() == 1);
    REQUIRE(s.series[0].points.size() == 1);
    CHECK(s.series[0].points[0].agents == 50);
    CHECK(s.rows == 1);
    CHECK_FALSE(s.hasLowerBounds); // soc_lb unknown

    // 16-column header (before soc_lb existed)
    auto oldHeader = header();
    oldHeader.resize(16);
    auto oldRow = row("cbs", 10, "square", "", true, 0.5, 42, -1, "solved");
    oldRow.resize(16);
    s = ResultsSummary::fromRows(oldHeader, {oldRow});
    REQUIRE(s.series.size() == 1);
    CHECK(s.series[0].points[0].meanCost == Catch::Approx(42.0));
    CHECK(std::isnan(s.series[0].points[0].meanCostOverLb));
}

TEST_CASE("summary of a missing file is empty", "[summary]")
{
    const auto s = ResultsSummary::fromCsv("/nonexistent/results.csv");
    CHECK(s.series.empty());
    CHECK(s.rows == 0);
}
