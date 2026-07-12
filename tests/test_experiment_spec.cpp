#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <set>

#include "ExperimentSpec.h"
#include "TestHelpers.h"

namespace {

// Writes a spec plus dummy map/scen files into a TempDir and returns the
// spec path. Instance paths inside the spec are relative to the dir.
std::string writeSpec(const TempDir& dir, const std::string& body)
{
    {
        std::ofstream m(dir.path() + "/a.map");
        m << "type octile\n";
    }
    {
        std::ofstream s(dir.path() + "/a.scen");
        s << "version 1\n";
    }
    {
        std::ofstream m(dir.path() + "/b.map");
        m << "type octile\n";
    }
    {
        std::ofstream s(dir.path() + "/b.scen");
        s << "version 1\n";
    }
    const std::string path = dir.path() + "/experiment.yaml";
    std::ofstream out(path);
    out << body;
    return path;
}

const char* kBasicSpec = "name: smoke\n"
                         "time_limit_s: 5\n"
                         "seeds: [0, 1]\n"
                         "agents: [2, 4]\n"
                         "solvers: [cbs, lacam2]\n"
                         "grids: [square, hex-oddr]\n"
                         "instances:\n"
                         "  - {map: a.map, scen: a.scen}\n"
                         "  - {map: b.map, scen: b.scen}\n";

} // namespace

TEST_CASE("spec parses and resolves paths against its own directory", "[experiment]")
{
    TempDir dir;
    auto spec = ExperimentSpec::loadFromFile(writeSpec(dir, kBasicSpec));

    CHECK(spec.name == "smoke");
    CHECK(spec.timeLimitS == 5);
    CHECK(spec.seeds == std::vector<int>{0, 1});
    CHECK(spec.agentLadder == std::vector<int>{2, 4});
    CHECK(spec.solvers == std::vector<std::string>{"cbs", "lacam2"});
    REQUIRE(spec.instances.size() == 2);
    CHECK(spec.instances[0].mapPath == dir.path() + "/a.map");
    CHECK(spec.instances[1].scenPath == dir.path() + "/b.scen");
    // default output_dir is spec-relative
    CHECK(spec.outputDir == dir.path() + "/experiments/smoke");
}

TEST_CASE("matrix expansion is solvers x instances x grids x agents x seeds", "[experiment]")
{
    TempDir dir;
    auto spec = ExperimentSpec::loadFromFile(writeSpec(dir, kBasicSpec));
    auto tasks = expandMatrix(spec);

    CHECK(tasks.size() == 2 * 2 * 2 * 2 * 2); // 32

    // all keys unique
    std::set<std::string> keys;
    for (const auto& t : tasks) {
        keys.insert(t.key());
    }
    CHECK(keys.size() == tasks.size());

    // derived naming and grid decomposition
    const auto& t0 = tasks.front();
    CHECK(t0.solver == "cbs");
    CHECK(t0.mapName() == "a");
    CHECK(t0.scenName() == "a.scen");
    CHECK(t0.gridType() == "square");
    CHECK(t0.hexLayout().empty());

    bool sawHex = false;
    for (const auto& t : tasks) {
        if (t.grid == "hex-oddr") {
            sawHex = true;
            CHECK(t.gridType() == "hex");
            CHECK(t.hexLayout() == "oddr");
        }
    }
    CHECK(sawHex);
}

TEST_CASE("agent ladder and seed-count shorthands expand", "[experiment]")
{
    TempDir dir;
    auto spec = ExperimentSpec::loadFromFile(writeSpec(dir, "name: ladder\n"
                                                            "agents: {from: 5, to: 20, step: 5}\n"
                                                            "seeds: 3\n"
                                                            "solvers: [cbs]\n"
                                                            "instances:\n"
                                                            "  - {map: a.map, scen: a.scen}\n"));

    CHECK(spec.agentLadder == std::vector<int>{5, 10, 15, 20});
    CHECK(spec.seeds == std::vector<int>{0, 1, 2});
    CHECK(spec.grids == std::vector<std::string>{"square"}); // default
    CHECK(expandMatrix(spec).size() == 4 * 3);
}

TEST_CASE("spec errors are actionable", "[experiment]")
{
    TempDir dir;

    auto expectError = [&](const std::string& body, const std::string& needle) {
        const std::string path = writeSpec(dir, body);
        try {
            ExperimentSpec::loadFromFile(path);
            FAIL("expected spec error containing '" << needle << "'");
        } catch (const std::runtime_error& e) {
            INFO(e.what());
            CHECK(std::string(e.what()).find(needle) != std::string::npos);
        }
    };

    expectError("agents: [2]\nsolvers: [cbs]\ninstances: [{map: a.map, scen: a.scen}]\n", "name");
    expectError("name: x\nagents: [2]\ninstances: [{map: a.map, scen: a.scen}]\n", "solvers");
    expectError("name: x\nsolvers: [cbs]\ninstances: [{map: a.map, scen: a.scen}]\n", "agents");
    expectError("name: x\nagents: [2]\nsolvers: [cbs]\n", "instances");
    expectError("name: x\nagents: [0]\nsolvers: [cbs]\n"
                "instances: [{map: a.map, scen: a.scen}]\n",
                ">= 1");
    expectError("name: x\nagents: [2]\nsolvers: [cbs]\ngrids: [triangular]\n"
                "instances: [{map: a.map, scen: a.scen}]\n",
                "unknown grid");
    expectError("name: x\nagents: [2]\nsolvers: [cbs]\n"
                "instances: [{map: missing.map, scen: a.scen}]\n",
                "map file not found");
    CHECK_THROWS(ExperimentSpec::loadFromFile("/nonexistent/spec.yaml"));
}

TEST_CASE("shuffle is deterministic for a fixed seed", "[experiment]")
{
    TempDir dir;
    auto spec = ExperimentSpec::loadFromFile(writeSpec(dir, kBasicSpec));

    auto a = expandMatrix(spec);
    auto b = expandMatrix(spec);
    shuffleTasks(a, 0xE1E1);
    shuffleTasks(b, 0xE1E1);

    REQUIRE(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].key() == b[i].key());
    }

    // A different seed produces a different order (for this matrix size)
    auto c = expandMatrix(spec);
    shuffleTasks(c, 0xBEEF);
    bool differs = false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].key() != c[i].key()) {
            differs = true;
            break;
        }
    }
    CHECK(differs);

    // Shuffle is a permutation: same key multiset
    std::set<std::string> ka, kc;
    for (const auto& t : a)
        ka.insert(t.key());
    for (const auto& t : c)
        kc.insert(t.key());
    CHECK(ka == kc);
}

TEST_CASE("task keys match CSV key extraction", "[experiment]")
{
    RunTask t;
    t.solver = "cbs";
    t.mapPath = "/x/empty-8-8.map";
    t.scenPath = "/x/empty-8-8-random-1.scen";
    t.grid = "hex-evenq";
    t.agents = 10;
    t.seed = 3;
    t.timeLimitS = 30;

    std::vector<std::string> csvRow = {"cbs",   "empty-8-8", "empty-8-8-random-1.scen",
                                       "10",    "hex",       "evenq",
                                       "3",     "30",        "1",
                                       "0.5",   "12",        "4",
                                       "100",   "2000",      "0.1.0",
                                       "solved"};
    CHECK(t.key() == RunTask::keyFromCsvRow(csvRow));
}
