#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "ResultsCsv.h"
#include "TestHelpers.h"

namespace {

std::vector<std::string> readLines(const std::string& path)
{
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        lines.push_back(line);
    }
    return lines;
}

RunRecord sampleRecord()
{
    RunRecord r;
    r.solver = "CBS";
    r.map = "random-32-32-10";
    r.scenario = "random-32-32-10-random-1.scen";
    r.agents = 20;
    r.gridType = "hex";
    r.hexLayout = "oddr";
    r.seed = 42;
    r.timeLimitS = 60;
    r.solved = true;
    r.runtimeS = 1.234567;
    r.solutionCost = 512;
    r.makespan = 40;
    r.hlExpanded = 120;
    r.llExpanded = 3400;
    r.toolVersion = "0.1.0";
    return r;
}

} // namespace

TEST_CASE("results CSV header is the documented stable schema", "[results]")
{
    CHECK(std::string(ResultsCsv::kHeader) ==
          "solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,"
          "solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version");
}

TEST_CASE("formatRow emits one value per header column", "[results]")
{
    CHECK(ResultsCsv::formatRow(sampleRecord()) ==
          "CBS,random-32-32-10,random-32-32-10-random-1.scen,20,hex,oddr,42,60,"
          "1,1.234567,512,40,120,3400,0.1.0");

    // Unknown metrics stay -1; unsolved is 0
    RunRecord r;
    r.solver = "LaCAM";
    r.toolVersion = "0.1.0";
    CHECK(ResultsCsv::formatRow(r) == "LaCAM,,,0,,,0,0,0,0.000000,-1,-1,-1,-1,0.1.0");
}

TEST_CASE("CSV escaping quotes fields with separators and quotes", "[results]")
{
    CHECK(ResultsCsv::escapeField("plain") == "plain");
    CHECK(ResultsCsv::escapeField("has,comma") == "\"has,comma\"");
    CHECK(ResultsCsv::escapeField("has \"quote\"") == "\"has \"\"quote\"\"\"");
    CHECK(ResultsCsv::escapeField("line\nbreak") == "\"line\nbreak\"");

    RunRecord r = sampleRecord();
    r.map = "maps/my, map.map";
    const std::string row = ResultsCsv::formatRow(r);
    CHECK(row.find("\"maps/my, map.map\"") != std::string::npos);
}

TEST_CASE("appendRow writes the header exactly once and flushes rows", "[results]")
{
    TempDir dir;
    const std::string csv = dir.path() + "/results.csv";

    REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
    REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));

    auto lines = readLines(csv);
    REQUIRE(lines.size() == 3);
    CHECK(lines[0] == ResultsCsv::kHeader);
    CHECK(lines[1] == lines[2]);
    CHECK(lines[1] == ResultsCsv::formatRow(sampleRecord()));
}

TEST_CASE("appendRow appends to existing non-empty files without re-writing the header",
          "[results]")
{
    TempDir dir;
    const std::string csv = dir.path() + "/results.csv";
    {
        std::ofstream out(csv);
        out << ResultsCsv::kHeader << "\npre-existing,row,x,0,,,0,0,0,0.0,-1,-1,-1,-1,0.0.9\n";
    }

    REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));

    auto lines = readLines(csv);
    REQUIRE(lines.size() == 3);
    CHECK(lines[0] == ResultsCsv::kHeader);
    CHECK(lines[2] == ResultsCsv::formatRow(sampleRecord()));
}

TEST_CASE("appendRow reports failure for unwritable destinations", "[results]")
{
    CHECK_FALSE(ResultsCsv::appendRow("/nonexistent-dir/results.csv", sampleRecord()));
}
