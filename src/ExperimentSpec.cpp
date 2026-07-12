#include "ExperimentSpec.h"

#include <algorithm>
#include <filesystem>
#include <random>
#include <sstream>
#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;

namespace {

[[noreturn]] void specError(const std::string& path, const std::string& what)
{
    throw std::runtime_error("experiment spec '" + path + "': " + what);
}

const std::vector<std::string> kValidGrids = {"square", "hex-oddr", "hex-evenr", "hex-oddq",
                                              "hex-evenq"};

std::string resolveAgainst(const fs::path& baseDir, const std::string& p)
{
    fs::path path(p);
    if (path.is_absolute()) {
        return path.lexically_normal().string();
    }
    return (baseDir / path).lexically_normal().string();
}

// agents: either a list ([2, 4, 8]) or a ladder map ({from: 5, to: 20, step: 5})
std::vector<int> parseAgents(const YAML::Node& node, const std::string& path)
{
    std::vector<int> out;
    if (node.IsSequence()) {
        for (const auto& v : node) {
            out.push_back(v.as<int>());
        }
    } else if (node.IsMap()) {
        const int from = node["from"].as<int>(0);
        const int to = node["to"].as<int>(-1);
        const int step = node["step"].as<int>(1);
        if (from <= 0 || to < from || step <= 0) {
            specError(path, "'agents' ladder needs from >= 1, to >= from, step >= 1");
        }
        for (int k = from; k <= to; k += step) {
            out.push_back(k);
        }
    } else {
        specError(path, "'agents' must be a list of counts or {from, to, step}");
    }
    if (out.empty()) {
        specError(path, "'agents' expands to an empty list");
    }
    for (int k : out) {
        if (k <= 0) {
            specError(path, "agent counts must be >= 1");
        }
    }
    return out;
}

// seeds: a list ([0, 1, 2]) or a scalar N meaning seeds 0..N-1
std::vector<int> parseSeeds(const YAML::Node& node, const std::string& path)
{
    std::vector<int> out;
    if (!node) {
        return {0};
    }
    if (node.IsSequence()) {
        for (const auto& v : node) {
            out.push_back(v.as<int>());
        }
    } else if (node.IsScalar()) {
        const int n = node.as<int>();
        if (n <= 0) {
            specError(path, "'seeds: N' requires N >= 1");
        }
        for (int i = 0; i < n; ++i) {
            out.push_back(i);
        }
    } else {
        specError(path, "'seeds' must be a list or a count");
    }
    if (out.empty()) {
        specError(path, "'seeds' is empty");
    }
    return out;
}

} // namespace

ExperimentSpec ExperimentSpec::loadFromFile(const std::string& path)
{
    YAML::Node root;
    try {
        root = YAML::LoadFile(path);
    } catch (const std::exception& e) {
        specError(path, std::string("cannot load: ") + e.what());
    }

    const fs::path absSpec = fs::absolute(path).lexically_normal();
    const fs::path baseDir = absSpec.parent_path();

    ExperimentSpec spec;
    spec.specPath = absSpec.string();

    spec.name = root["name"].as<std::string>("");
    if (spec.name.empty()) {
        specError(path, "missing required field 'name'");
    }

    spec.timeLimitS = root["time_limit_s"].as<int>(30);
    if (spec.timeLimitS <= 0) {
        specError(path, "'time_limit_s' must be >= 1");
    }

    spec.shuffleSeed = root["shuffle_seed"].as<std::uint32_t>(0);

    spec.outputDir =
        resolveAgainst(baseDir, root["output_dir"].as<std::string>("experiments/" + spec.name));

    if (!root["solvers"] || !root["solvers"].IsSequence() || root["solvers"].size() == 0) {
        specError(path, "missing required list 'solvers'");
    }
    for (const auto& s : root["solvers"]) {
        spec.solvers.push_back(s.as<std::string>());
    }

    if (!root["agents"]) {
        specError(path, "missing required field 'agents'");
    }
    spec.agentLadder = parseAgents(root["agents"], path);

    spec.seeds = parseSeeds(root["seeds"], path);

    if (root["grids"]) {
        for (const auto& g : root["grids"]) {
            spec.grids.push_back(g.as<std::string>());
        }
    } else {
        spec.grids = {"square"};
    }
    for (const auto& g : spec.grids) {
        if (std::find(kValidGrids.begin(), kValidGrids.end(), g) == kValidGrids.end()) {
            std::ostringstream msg;
            msg << "unknown grid '" << g << "' (valid:";
            for (const auto& v : kValidGrids) {
                msg << ' ' << v;
            }
            msg << ")";
            specError(path, msg.str());
        }
    }

    if (!root["instances"] || !root["instances"].IsSequence() || root["instances"].size() == 0) {
        specError(path, "missing required list 'instances'");
    }
    for (const auto& node : root["instances"]) {
        ExperimentInstance inst;
        inst.mapPath = resolveAgainst(baseDir, node["map"].as<std::string>(""));
        inst.scenPath = resolveAgainst(baseDir, node["scen"].as<std::string>(""));
        if (node["map"].as<std::string>("").empty() || node["scen"].as<std::string>("").empty()) {
            specError(path, "each instance needs both 'map' and 'scen'");
        }
        if (!fs::is_regular_file(inst.mapPath)) {
            specError(path, "map file not found: " + inst.mapPath);
        }
        if (!fs::is_regular_file(inst.scenPath)) {
            specError(path, "scen file not found: " + inst.scenPath);
        }
        spec.instances.push_back(std::move(inst));
    }

    return spec;
}

std::string RunTask::mapName() const
{
    return fs::path(mapPath).stem().string();
}

std::string RunTask::scenName() const
{
    return fs::path(scenPath).filename().string();
}

std::string RunTask::gridType() const
{
    return grid == "square" ? "square" : "hex";
}

std::string RunTask::hexLayout() const
{
    if (grid.rfind("hex-", 0) == 0) {
        return grid.substr(4);
    }
    return "";
}

std::string RunTask::key() const
{
    std::ostringstream k;
    k << solver << '\x1f' << mapName() << '\x1f' << scenName() << '\x1f' << agents << '\x1f'
      << gridType() << '\x1f' << hexLayout() << '\x1f' << seed << '\x1f' << timeLimitS;
    return k.str();
}

std::string RunTask::keyFromCsvRow(const std::vector<std::string>& fields)
{
    std::string k;
    for (std::size_t i = 0; i < 8 && i < fields.size(); ++i) {
        if (i > 0) {
            k += '\x1f';
        }
        k += fields[i];
    }
    return k;
}

std::vector<RunTask> expandMatrix(const ExperimentSpec& spec)
{
    std::vector<RunTask> tasks;
    for (const auto& solver : spec.solvers) {
        for (const auto& inst : spec.instances) {
            for (const auto& grid : spec.grids) {
                for (int agents : spec.agentLadder) {
                    for (int seed : spec.seeds) {
                        RunTask t;
                        t.solver = solver;
                        t.mapPath = inst.mapPath;
                        t.scenPath = inst.scenPath;
                        t.grid = grid;
                        t.agents = agents;
                        t.seed = seed;
                        t.timeLimitS = spec.timeLimitS;
                        tasks.push_back(std::move(t));
                    }
                }
            }
        }
    }
    return tasks;
}

void shuffleTasks(std::vector<RunTask>& tasks, std::uint32_t seed)
{
    // Explicit Fisher-Yates with mt19937 and modulo indexing: identical
    // sequence on every platform/toolchain (std::shuffle's distribution is
    // implementation-defined).
    std::mt19937 rng(seed);
    for (std::size_t i = tasks.size(); i > 1; --i) {
        std::size_t j = static_cast<std::size_t>(rng() % i);
        std::swap(tasks[i - 1], tasks[j]);
    }
}
