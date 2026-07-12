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
    r.status = "solved";
    return r;
}

} // namespace

TEST_CASE("results CSV header is the documented stable schema", "[results]")
{
    CHECK(std::string(ResultsCsv::kHeader) ==
          "solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,"
          "solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version,status");
}

TEST_CASE("formatRow emits one value per header column", "[results]")
{
    CHECK(ResultsCsv::formatRow(sampleRecord()) ==
          "CBS,random-32-32-10,random-32-32-10-random-1.scen,20,hex,oddr,42,60,"
          "1,1.234567,512,40,120,3400,0.1.0,solved");

    // Unknown metrics stay -1; unsolved is 0
    RunRecord r;
    r.solver = "LaCAM";
    r.toolVersion = "0.1.0";
    CHECK(ResultsCsv::formatRow(r) == "LaCAM,,,0,,,0,0,0,0.000000,-1,-1,-1,-1,0.1.0,");
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
        out << ResultsCsv::kHeader << "\npre-existing,row,x,0,,,0,0,0,0.0,-1,-1,-1,-1,0.0.9,ok\n";
    }

    REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));

    auto lines = readLines(csv);
    REQUIRE(lines.size() == 3);
    CHECK(lines[0] == ResultsCsv::kHeader);
    CHECK(lines[2] == ResultsCsv::formatRow(sampleRecord()));
}

TEST_CASE("splitRow honors RFC-4180 quoting", "[results]")
{
    auto f = ResultsCsv::splitRow("a,\"b,c\",\"d\"\"e\",,f");
    REQUIRE(f.size() == 5);
    CHECK(f[0] == "a");
    CHECK(f[1] == "b,c");
    CHECK(f[2] == "d\"e");
    CHECK(f[3] == "");
    CHECK(f[4] == "f");

    // formatRow -> splitRow round-trip
    RunRecord r = sampleRecord();
    r.map = "my, \"quoted\" map";
    auto fields = ResultsCsv::splitRow(ResultsCsv::formatRow(r));
    REQUIRE(fields.size() == ResultsCsv::kColumnCount);
    CHECK(fields[1] == "my, \"quoted\" map");
    CHECK(fields[15] == "solved");
}

TEST_CASE("readCompletedRows skips the header and truncated final rows", "[results]")
{
    TempDir dir;
    const std::string csv = dir.path() + "/results.csv";

    SECTION("missing file yields no rows")
    {
        CHECK(ResultsCsv::readCompletedRows(csv).empty());
    }

    SECTION("complete rows only")
    {
        REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
        REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
        auto rows = ResultsCsv::readCompletedRows(csv);
        REQUIRE(rows.size() == 2);
        CHECK(rows[0][0] == "CBS");
        CHECK(rows[0].size() == ResultsCsv::kColumnCount);
    }

    SECTION("a final line without a newline (crash mid-write) is ignored")
    {
        REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
        {
            std::ofstream out(csv, std::ios::app);
            out << "LaCAM,map,scen,5,square,,0,30,1,0.5"; // truncated, no newline
        }
        auto rows = ResultsCsv::readCompletedRows(csv);
        REQUIRE(rows.size() == 1);
        CHECK(rows[0][0] == "CBS");

        // Appending after a crash first terminates the partial row, so the
        // new row is intact. The repaired partial row still has fewer
        // fields than the header and stays ignored.
        REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
        rows = ResultsCsv::readCompletedRows(csv);
        REQUIRE(rows.size() == 2);
        CHECK(rows[1][0] == "CBS");
        CHECK(rows[1].size() == ResultsCsv::kColumnCount);
    }

    SECTION("short garbage rows are ignored")
    {
        REQUIRE(ResultsCsv::appendRow(csv, sampleRecord()));
        {
            std::ofstream out(csv, std::ios::app);
            out << "only,three,fields\n";
        }
        auto rows = ResultsCsv::readCompletedRows(csv);
        REQUIRE(rows.size() == 1);
    }

    SECTION("header-only file yields no rows")
    {
        std::ofstream out(csv);
        out << ResultsCsv::kHeader << "\n";
        out.close();
        CHECK(ResultsCsv::readCompletedRows(csv).empty());
    }
}

TEST_CASE("appendRow reports failure for unwritable destinations", "[results]")
{
    CHECK_FALSE(ResultsCsv::appendRow("/nonexistent-dir/results.csv", sampleRecord()));
}
