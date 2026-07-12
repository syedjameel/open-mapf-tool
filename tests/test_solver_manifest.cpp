#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "ConfigLocator.h"
#include "Environment.h"
#include "SolverManifest.h"
#include "TestHelpers.h"

TEST_CASE("default manifest lists the four bundled solver drivers", "[manifest]")
{
    auto m = SolverManifest::withDefaults();

    REQUIRE(m.entries().size() == 4);
    for (const char* key : {"cbs", "lacam", "lacam2", "lacam3"}) {
        INFO("key: " << key);
        const SolverEntry* e = m.find(key);
        REQUIRE(e != nullptr);
        CHECK(e->executable.empty()); // nothing hardcoded
        CHECK_FALSE(e->name.empty());
        CHECK_FALSE(e->grids.empty());
    }
    CHECK(m.executableFor("nonexistent").empty());
}

TEST_CASE("loadFromFile merges into defaults", "[manifest]")
{
    TempFile file(
        "solvers:\n"
        "  - key: cbs\n"
        "    executable: /opt/solvers/cbs\n"
        "  - key: my-custom-solver\n"
        "    name: My Custom Solver\n"
        "    executable: /opt/solvers/custom\n"
        "    grids: [square]\n",
        ".yaml");

    auto m = SolverManifest::withDefaults();
    REQUIRE(m.loadFromFile(file.path()));

    CHECK(m.executableFor("cbs") == "/opt/solvers/cbs");
    CHECK(m.find("cbs")->name == "CBSH2-RTC-CHBP-HEX"); // untouched fields kept

    const SolverEntry* custom = m.find("my-custom-solver");
    REQUIRE(custom != nullptr); // unknown keys are appended
    CHECK(custom->name == "My Custom Solver");
    CHECK(custom->grids == std::vector<std::string>{"square"});

    CHECK(m.executableFor("lacam").empty()); // unrelated entries untouched
}

TEST_CASE("template files with empty executables never erase configured paths", "[manifest]")
{
    auto m = SolverManifest::withDefaults();
    m.find("cbs")->executable = "/configured/cbs";

    TempFile tmpl(
        "solvers:\n"
        "  - key: cbs\n"
        "    executable: \"\"\n",
        ".yaml");
    REQUIRE(m.loadFromFile(tmpl.path()));

    CHECK(m.executableFor("cbs") == "/configured/cbs");
}

TEST_CASE("manifest load failures leave the manifest usable", "[manifest]")
{
    auto m = SolverManifest::withDefaults();

    SECTION("missing file")
    {
        CHECK_FALSE(m.loadFromFile("/nonexistent/solvers.yaml"));
    }

    SECTION("malformed yaml")
    {
        TempFile bad("solvers: [unbalanced", ".yaml");
        CHECK_FALSE(m.loadFromFile(bad.path()));
    }

    SECTION("no solvers key")
    {
        TempFile bad("something_else: 1\n", ".yaml");
        CHECK_FALSE(m.loadFromFile(bad.path()));
    }

    CHECK(m.entries().size() == 4);
}

TEST_CASE("manifest save/load round-trips", "[manifest]")
{
    auto m = SolverManifest::withDefaults();
    m.find("cbs")->executable = "/opt/round/trip cbs"; // path with a space
    m.find("lacam2")->executable = "/opt/round/lacam2";

    TempDir dir;
    std::string path = dir.path() + "/nested/solvers.yaml"; // exercises mkdir -p
    REQUIRE(m.saveToFile(path));

    auto loaded = SolverManifest::withDefaults();
    REQUIRE(loaded.loadFromFile(path));

    CHECK(loaded.executableFor("cbs") == "/opt/round/trip cbs");
    CHECK(loaded.executableFor("lacam2") == "/opt/round/lacam2");
    CHECK(loaded.executableFor("lacam").empty());
    CHECK(loaded.entries().size() == 4);
}

TEST_CASE("legacy env.yaml solver keys fill unconfigured entries only", "[manifest]")
{
    Environment env;
    env.m_solver_cbs = "/legacy/cbs";
    env.m_solver_lacam = "/legacy/lacam";

    auto m = SolverManifest::withDefaults();
    m.find("cbs")->executable = "/manifest/cbs"; // manifest wins

    m.applyLegacyEnvironment(env);

    CHECK(m.executableFor("cbs") == "/manifest/cbs");
    CHECK(m.executableFor("lacam") == "/legacy/lacam");
    CHECK(m.executableFor("lacam2").empty());
}

TEST_CASE("solvers.yaml is discovered through the same location chain", "[manifest][config]")
{
    ConfigLocator::Inputs in;
    in.xdgConfigHome = "/xdg";
    in.exeDir = "/opt/openmapf/bin";

    auto candidates = ConfigLocator::candidatePaths(in, "solvers.yaml");
    REQUIRE(candidates.size() == 4);
    CHECK(candidates[0] == "/xdg/openmapf/solvers.yaml");
    CHECK(candidates[1] == "/opt/openmapf/bin/../share/openmapf/solvers.yaml");
    CHECK(candidates[2] == "/opt/openmapf/bin/../config/solvers.yaml");
    CHECK(candidates[3] == "/opt/openmapf/bin/config/solvers.yaml");

    CHECK(ConfigLocator::userConfigDir(in) == "/xdg/openmapf");
}
