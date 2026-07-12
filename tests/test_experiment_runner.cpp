// End-to-end tests for the batch experiment runner, driven exclusively by
// fake solver shell scripts (sleep/echo) -- no real solver binaries.
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <unistd.h>

#include <yaml-cpp/yaml.h>

#include "ExperimentRunner.h"
#include "TestHelpers.h"

namespace fs = std::filesystem;

namespace {

const char* kFakeCbsScript =
    "#!/bin/sh\n"
    "out=\"\"; paths=\"\"\n"
    "while [ $# -gt 0 ]; do\n"
    "  case \"$1\" in\n"
    "    -o) out=\"$2\"; shift 2;;\n"
    "    --outputPaths=*) paths=\"${1#--outputPaths=}\"; shift;;\n"
    "    *) shift;;\n"
    "  esac\n"
    "done\n"
    "echo 'runtime,hl expanded,hl generated,ll expanded,ll generated,solution cost' > \"$out\"\n"
    "echo '0.25,5,9,50,90,11' >> \"$out\"\n"
    "printf 'Agent 0: (0,0)->(1,1)\\n' > \"$paths\"\n";

const char* kFakeLacamScript =
    "#!/bin/sh\n"
    "out=\"\"\n"
    "while [ $# -gt 0 ]; do\n"
    "  case \"$1\" in\n"
    "    -o) out=\"$2\"; shift 2;;\n"
    "    *) shift;;\n"
    "  esac\n"
    "done\n"
    "cat > \"$out\" <<'EOF'\n"
    "solved=1\n"
    "soc=9\n"
    "makespan=3\n"
    "comp_time=125\n"
    "solution=\n"
    "0:(0,0),(1,1),\n"
    "1:(1,0),(2,1),\n"
    "EOF\n";

// Workspace: tiny map + scen + spec, fake solver scripts, manifest.
struct Workspace {
    TempDir dir;
    TempScript fakeCbs{kFakeCbsScript};
    TempScript fakeLacam{kFakeLacamScript};

    Workspace()
    {
        std::ofstream map(dir.path() + "/tiny.map");
        map << "type octile\nheight 4\nwidth 4\nmap\n....\n....\n....\n....\n";

        std::ofstream scen(dir.path() + "/tiny.scen");
        scen << "version 1\n"
             << "0\ttiny.map\t4\t4\t0\t0\t3\t3\t0\n"
             << "0\ttiny.map\t4\t4\t1\t0\t2\t3\t0\n"
             << "0\ttiny.map\t4\t4\t2\t0\t1\t3\t0\n"
             << "0\ttiny.map\t4\t4\t3\t0\t0\t3\t0\n";
    }

    std::string writeSpec(const std::string& body)
    {
        const std::string path = dir.path() + "/experiment.yaml";
        std::ofstream out(path);
        out << body;
        return path;
    }

    SolverManifest manifest() const
    {
        auto m = SolverManifest::withDefaults();
        m.find("cbs")->executable = fakeCbs.path();
        m.find("lacam2")->executable = fakeLacam.path();
        return m;
    }
};

RunnerOptions testOptions()
{
    RunnerOptions opts;
    opts.jobs = 2;
    opts.toolVersion = "test-0.0";
    return opts;
}

std::string kSmokeSpec =
    "name: smoke\n"
    "output_dir: out\n"
    "time_limit_s: 5\n"
    "seeds: [0, 1]\n"
    "agents: [1, 2]\n"
    "solvers: [cbs, lacam2]\n"
    "instances:\n"
    "  - {map: tiny.map, scen: tiny.scen}\n";

} // namespace

TEST_CASE("runner executes the full matrix with fake solvers", "[runner]")
{
    Workspace ws;
    auto spec = ExperimentSpec::loadFromFile(ws.writeSpec(kSmokeSpec));
    ExperimentRunner runner(spec, ws.manifest(), testOptions());

    REQUIRE(runner.validate().empty());
    REQUIRE(runner.pendingTasks().size() == 8); // 2 solvers x 2 agents x 2 seeds

    std::size_t progressCalls = 0;
    RunnerProgress last;
    REQUIRE(runner.run([&](const RunnerProgress& p) {
                ++progressCalls;
                last = p;
            }) == 0);

    CHECK(progressCalls == 8);
    CHECK(last.completed == 8);
    CHECK(last.total == 8);

    // Results: 8 complete rows, unique keys, all solved
    auto rows = ResultsCsv::readCompletedRows(runner.resultsCsvPath());
    REQUIRE(rows.size() == 8);
    std::set<std::string> keys;
    for (const auto& row : rows) {
        keys.insert(RunTask::keyFromCsvRow(row));
        CHECK(row[15] == "solved");
        CHECK(row[14] == "test-0.0"); // tool_version
    }
    CHECK(keys.size() == 8);

    // CBS rows carry hl/ll; LaCAM rows carry makespan
    for (const auto& row : rows) {
        if (row[0] == "cbs") {
            CHECK(row[10] == "11"); // solution_cost
            CHECK(row[12] == "5");  // hl_expanded
        } else {
            CHECK(row[0] == "lacam2");
            CHECK(row[11] == "3"); // makespan
        }
    }

    // Manifest provenance
    YAML::Node manifest = YAML::LoadFile(runner.manifestPath());
    CHECK(manifest["experiment"].as<std::string>() == "smoke");
    CHECK(manifest["tool_version"].as<std::string>() == "test-0.0");
    CHECK_FALSE(manifest["started"].as<std::string>().empty());
    CHECK_FALSE(manifest["finished"].as<std::string>().empty());
    CHECK(manifest["solvers"].size() == 2);
    CHECK(manifest["spec_copy"].as<std::string>().find("name: smoke") != std::string::npos);
    CHECK(manifest["pending_at_start"].as<std::size_t>() == 8);

    // Per-invocation temp tree cleaned up
    const std::string tempRoot =
        (fs::temp_directory_path() / ("openmapf-exp-" + std::to_string(::getpid()))).string();
    CHECK_FALSE(fs::exists(tempRoot));
}

TEST_CASE("runner resumes by key and refuses accidental overwrite", "[runner]")
{
    Workspace ws;
    auto spec = ExperimentSpec::loadFromFile(ws.writeSpec(kSmokeSpec));

    {
        ExperimentRunner first(spec, ws.manifest(), testOptions());
        REQUIRE(first.run() == 0);
        REQUIRE(ResultsCsv::readCompletedRows(first.resultsCsvPath()).size() == 8);
    }

    SECTION("without --resume, existing results are refused")
    {
        ExperimentRunner again(spec, ws.manifest(), testOptions());
        CHECK(again.run() == 2);
    }

    SECTION("with --resume and everything done, nothing runs")
    {
        auto opts = testOptions();
        opts.resume = true;
        ExperimentRunner again(spec, ws.manifest(), opts);
        CHECK(again.pendingTasks().empty());
        CHECK(again.run() == 0);
        CHECK(ResultsCsv::readCompletedRows(again.resultsCsvPath()).size() == 8);
    }

    SECTION("with --resume after a crash (truncated final row), only missing runs execute")
    {
        // Keep header + first 3 complete rows, then a truncated 4th row.
        const std::string csvPath = spec.outputDir + "/results.csv";
        std::ifstream in(csvPath);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line)) {
            lines.push_back(line);
        }
        in.close();
        REQUIRE(lines.size() == 9);
        {
            std::ofstream out(csvPath, std::ios::trunc);
            for (int i = 0; i <= 3; ++i) {
                out << lines[static_cast<std::size_t>(i)] << "\n";
            }
            out << lines[4].substr(0, lines[4].size() / 2); // no newline
        }

        auto opts = testOptions();
        opts.resume = true;
        ExperimentRunner again(spec, ws.manifest(), opts);
        CHECK(again.pendingTasks().size() == 5); // 8 - 3 complete
        REQUIRE(again.run() == 0);

        auto rows = ResultsCsv::readCompletedRows(csvPath);
        REQUIRE(rows.size() == 8);
        std::set<std::string> keys;
        for (const auto& row : rows) {
            keys.insert(RunTask::keyFromCsvRow(row));
        }
        CHECK(keys.size() == 8); // no duplicates
    }
}

TEST_CASE("runner kills hung solvers at the hard deadline", "[runner]")
{
    Workspace ws;
    TempScript hungSolver("#!/bin/sh\nsleep 60\n");
    auto manifest = ws.manifest();
    manifest.find("cbs")->executable = hungSolver.path();

    auto spec = ExperimentSpec::loadFromFile(ws.writeSpec(
        "name: timeout\n"
        "output_dir: out-timeout\n"
        "time_limit_s: 1\n"
        "agents: [1]\n"
        "solvers: [cbs]\n"
        "instances:\n"
        "  - {map: tiny.map, scen: tiny.scen}\n"));

    auto opts = testOptions();
    opts.hardTimeoutGraceS = 1; // hard kill at ~2s

    ExperimentRunner runner(spec, manifest, opts);
    const auto start = std::chrono::steady_clock::now();
    REQUIRE(runner.run() == 0);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    CHECK(elapsed < std::chrono::seconds(30)); // far less than the 60s sleep

    auto rows = ResultsCsv::readCompletedRows(runner.resultsCsvPath());
    REQUIRE(rows.size() == 1);
    CHECK(rows[0][15] == "hard_timeout");
    CHECK(rows[0][8] == "0"); // not solved
}

TEST_CASE("runner records skipped and error outcomes", "[runner]")
{
    Workspace ws;

    SECTION("scen with fewer agents than requested -> skipped")
    {
        auto spec = ExperimentSpec::loadFromFile(ws.writeSpec(
            "name: skip\noutput_dir: out-skip\nagents: [10]\nsolvers: [cbs]\n"
            "instances: [{map: tiny.map, scen: tiny.scen}]\n"));
        ExperimentRunner runner(spec, ws.manifest(), testOptions());
        REQUIRE(runner.run() == 0);
        auto rows = ResultsCsv::readCompletedRows(runner.resultsCsvPath());
        REQUIRE(rows.size() == 1);
        CHECK(rows[0][15] == "skipped");
    }

    SECTION("solver exiting non-zero -> error")
    {
        TempScript failing("#!/bin/sh\necho boom 1>&2\nexit 3\n");
        auto manifest = ws.manifest();
        manifest.find("cbs")->executable = failing.path();
        auto spec = ExperimentSpec::loadFromFile(ws.writeSpec(
            "name: err\noutput_dir: out-err\nagents: [1]\nsolvers: [cbs]\n"
            "instances: [{map: tiny.map, scen: tiny.scen}]\n"));
        ExperimentRunner runner(spec, manifest, testOptions());
        REQUIRE(runner.run() == 0); // attempted; failure recorded in the row
        auto rows = ResultsCsv::readCompletedRows(runner.resultsCsvPath());
        REQUIRE(rows.size() == 1);
        CHECK(rows[0][15] == "error");
    }
}

TEST_CASE("runner validation catches unusable solver configs", "[runner]")
{
    Workspace ws;

    auto specFor = [&](const std::string& solver, const std::string& outName) {
        return ExperimentSpec::loadFromFile(ws.writeSpec(
            "name: v\noutput_dir: " + outName + "\nagents: [1]\nsolvers: [" + solver + "]\n"
            "instances: [{map: tiny.map, scen: tiny.scen}]\n"));
    };

    SECTION("unknown driver family")
    {
        ExperimentRunner runner(specFor("mystery", "o1"), ws.manifest(), testOptions());
        auto errors = runner.validate();
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].find("no driver") != std::string::npos);
    }

    SECTION("unconfigured executable")
    {
        ExperimentRunner runner(specFor("lacam3", "o2"), ws.manifest(), testOptions());
        auto errors = runner.validate();
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].find("no executable configured") != std::string::npos);
    }

    SECTION("configured but missing executable")
    {
        auto manifest = ws.manifest();
        manifest.find("cbs")->executable = "/nonexistent/cbs";
        ExperimentRunner runner(specFor("cbs", "o3"), manifest, testOptions());
        auto errors = runner.validate();
        REQUIRE(errors.size() == 1);
        CHECK(errors[0].find("not found") != std::string::npos);
    }
}

TEST_CASE("defaultJobs is bounded", "[runner]")
{
    const int jobs = ExperimentRunner::defaultJobs();
    CHECK(jobs >= 1);
    CHECK(jobs <= 4);
}
