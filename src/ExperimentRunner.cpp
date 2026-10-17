#include "ExperimentRunner.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <unordered_set>

#include <yaml-cpp/yaml.h>

#include "CBSDriver.h"
#include "Environment.h"
#include "LacamDriver.h"
#include "MapFormat.h"
#include "ProcessRunner.h"

namespace fs = std::filesystem;

namespace {

const std::vector<std::string> kKnownDriverKeys = {"cbs", "lacam", "lacam2", "lacam3"};

std::string isoUtcNow()
{
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
    gmtime_r(&now, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string readWholeFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string trimmed(std::string s)
{
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
        s.pop_back();
    }
    return s;
}

// Human-readable id for progress lines.
std::string prettyKey(const RunTask& t)
{
    std::ostringstream k;
    k << t.solver << " " << t.mapName() << " " << t.grid << " k=" << t.agents << " seed=" << t.seed;
    return k.str();
}

} // namespace

ExperimentRunner::ExperimentRunner(ExperimentSpec spec, SolverManifest manifest, RunnerOptions opts)
    : m_spec(std::move(spec)), m_manifest(std::move(manifest)), m_opts(std::move(opts))
{
    if (m_opts.jobs <= 0) {
        m_opts.jobs = defaultJobs();
    }
}

int ExperimentRunner::defaultJobs()
{
    const unsigned cores = std::thread::hardware_concurrency();
    const unsigned half = cores > 1 ? cores / 2 : 1;
    return static_cast<int>(std::max(1u, std::min(4u, half)));
}

std::vector<std::string> ExperimentRunner::validate() const
{
    std::vector<std::string> errors;
    for (const auto& key : m_spec.solvers) {
        if (std::find(kKnownDriverKeys.begin(), kKnownDriverKeys.end(), key) ==
            kKnownDriverKeys.end()) {
            errors.push_back("solver '" + key +
                             "' has no driver (supported: cbs, lacam, lacam2, lacam3)");
            continue;
        }
        const std::string exe = m_manifest.executableFor(key);
        if (exe.empty()) {
            errors.push_back("solver '" + key +
                             "' has no executable configured (set it in solvers.yaml or run "
                             "scripts/get-solvers.sh)");
        } else if (!fs::is_regular_file(exe)) {
            errors.push_back("solver '" + key + "' executable not found: " + exe);
        }
    }
    return errors;
}

std::string ExperimentRunner::resultsCsvPath() const
{
    return (fs::path(m_spec.outputDir) / "results.csv").string();
}

std::string ExperimentRunner::manifestPath() const
{
    return (fs::path(m_spec.outputDir) / "manifest.yaml").string();
}

std::vector<RunTask> ExperimentRunner::pendingTasks() const
{
    std::vector<RunTask> tasks = expandMatrix(m_spec);
    shuffleTasks(tasks, m_spec.shuffleSeed);

    if (m_opts.resume) {
        std::unordered_set<std::string> done;
        for (const auto& row : ResultsCsv::readCompletedRows(resultsCsvPath())) {
            done.insert(RunTask::keyFromCsvRow(row));
        }
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                                   [&done](const RunTask& t) { return done.count(t.key()) != 0; }),
                    tasks.end());
    }
    return tasks;
}

void ExperimentRunner::requestStop()
{
    m_stop.store(true);
}

std::string ExperimentRunner::solverVersion(const std::string& executable) const
{
    // Never execute the solver itself just to learn its version (it might
    // start solving); ask git about its checkout instead, when there is one.
    std::error_code ec;
    const std::string dir = fs::path(executable).parent_path().string();
    if (dir.empty() || !fs::exists(dir, ec)) {
        return "unknown";
    }
    ProcessRunner git;
    auto r = git.run({"git", "-C", dir, "describe", "--always", "--dirty", "--tags"});
    if (r.started && r.exitCode == 0) {
        const std::string v = trimmed(r.stdoutText);
        if (!v.empty()) {
            return v;
        }
    }
    return "unknown";
}

void ExperimentRunner::writeManifest(const std::string& started, const std::string& finished,
                                     std::size_t totalTasks, std::size_t pendingCount) const
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "experiment" << YAML::Value << m_spec.name;
    out << YAML::Key << "tool_version" << YAML::Value << m_opts.toolVersion;
    out << YAML::Key << "spec_file" << YAML::Value << m_spec.specPath;
    out << YAML::Key << "output_dir" << YAML::Value << m_spec.outputDir;
    out << YAML::Key << "shuffle_seed" << YAML::Value << m_spec.shuffleSeed;
    out << YAML::Key << "jobs" << YAML::Value << m_opts.jobs;
    out << YAML::Key << "time_limit_s" << YAML::Value << m_spec.timeLimitS;
    out << YAML::Key << "hard_timeout_grace_s" << YAML::Value << m_opts.hardTimeoutGraceS;
    out << YAML::Key << "total_tasks" << YAML::Value << totalTasks;
    out << YAML::Key << "pending_at_start" << YAML::Value << pendingCount;
    out << YAML::Key << "started" << YAML::Value << started;
    out << YAML::Key << "finished" << YAML::Value << finished;

    out << YAML::Key << "solvers" << YAML::Value << YAML::BeginSeq;
    for (const auto& key : m_spec.solvers) {
        const std::string exe = m_manifest.executableFor(key);
        out << YAML::BeginMap;
        out << YAML::Key << "key" << YAML::Value << key;
        out << YAML::Key << "executable" << YAML::Value << exe;
        out << YAML::Key << "version" << YAML::Value << solverVersion(exe);
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "spec_copy" << YAML::Value << YAML::Literal
        << readWholeFile(m_spec.specPath);
    out << YAML::EndMap;

    std::ofstream file(manifestPath());
    file << "# OpenMAPF experiment provenance -- generated by the batch runner\n"
         << out.c_str() << "\n";
}

RunRecord ExperimentRunner::executeTask(const RunTask& task, const std::string& runTempDir)
{
    RunRecord rec;
    rec.solver = task.solver;
    rec.map = task.mapName();
    rec.scenario = task.scenName();
    rec.agents = static_cast<std::size_t>(task.agents);
    rec.gridType = task.gridType();
    rec.hexLayout = task.hexLayout();
    rec.seed = task.seed;
    rec.timeLimitS = task.timeLimitS;
    rec.toolVersion = m_opts.toolVersion;

    // --- instance setup -----------------------------------------------------
    Environment env;
    try {
        env.loadMap(task.mapPath);
    } catch (const std::exception& e) {
        std::cerr << "[run] failed to load map " << task.mapPath << ": " << e.what() << std::endl;
        rec.status = "error";
        return rec;
    }
    env.m_square = task.grid == "square";
    env.m_hexagon = !env.m_square;
    const std::string layout = task.hexLayout();
    if (layout == "oddr")
        env.m_hex_grid_type = HexGridType::ODD_R;
    else if (layout == "evenr")
        env.m_hex_grid_type = HexGridType::EVEN_R;
    else if (layout == "oddq")
        env.m_hex_grid_type = HexGridType::ODD_Q;
    else if (layout == "evenq")
        env.m_hex_grid_type = HexGridType::EVEN_Q;

    MapFormat::ScenFile scen;
    try {
        scen = MapFormat::loadScenFile(task.scenPath);
    } catch (const std::exception& e) {
        std::cerr << "[run] failed to load scen " << task.scenPath << ": " << e.what() << std::endl;
        rec.status = "error";
        return rec;
    }
    auto sel = MapFormat::selectAgents(scen.entries, env.m_grid, env.m_cols, env.m_rows,
                                       static_cast<std::size_t>(task.agents));
    if (sel.agents.size() < static_cast<std::size_t>(task.agents)) {
        rec.status = "skipped"; // instance cannot provide the requested k
        return rec;
    }

    const auto hardDeadline =
        std::chrono::seconds(task.timeLimitS + std::max(0, m_opts.hardTimeoutGraceS));
    const auto startTime = std::chrono::steady_clock::now();
    auto elapsedS = [&startTime]() {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime).count();
    };

    // --- drive the solver ----------------------------------------------------
    if (task.solver == "cbs") {
        CBSDriver driver;
        driver.setExecutablePath(m_manifest.executableFor(task.solver));

        CBSParameters params;
        params.cutoffTime = task.timeLimitS;
        params.seed = task.seed;
        params.hexGrid = env.m_hexagon;
        if (env.m_hexagon) {
            params.gridConfig = layout;
        }

        std::promise<CBSResults> promise;
        auto fut = promise.get_future();
        driver.solveAsync(
            sel.agents, env, params, task.mapName(),
            [&promise](const CBSResults& r) { promise.set_value(r); }, runTempDir);

        if (fut.wait_for(hardDeadline) != std::future_status::ready) {
            driver.cancel(); // kills the child; driver dtor joins the thread
            rec.status = "hard_timeout";
            rec.runtimeS = elapsedS();
            return rec;
        }
        CBSResults r = fut.get();
        rec.solved = r.solutionFound;
        rec.runtimeS = r.runtime;
        rec.solutionCost = r.solutionCost;
        rec.hlExpanded = r.highLevelNodes;
        rec.llExpanded = r.lowLevelNodes;
        rec.status = r.solutionFound ? "solved" : (r.errorMessage.empty() ? "unsolved" : "error");
        if (!r.errorMessage.empty()) {
            std::cerr << "[run] " << prettyKey(task) << ": " << r.errorMessage << std::endl;
        }
    } else {
        LacamDriver lacam;
        Lacam2Driver lacam2;
        Lacam3Driver lacam3;
        LacamDriverBase* driver = nullptr;
        if (task.solver == "lacam")
            driver = &lacam;
        else if (task.solver == "lacam2")
            driver = &lacam2;
        else
            driver = &lacam3;
        driver->setExecutablePath(m_manifest.executableFor(task.solver));

        LacamParameters params;
        params.timeLimit = task.timeLimitS;
        params.seed = task.seed;
        params.hexGrid = env.m_hexagon;

        std::promise<LacamResults> promise;
        auto fut = promise.get_future();
        driver->solveAsync(
            sel.agents, env, params, task.mapName(),
            [&promise](const LacamResults& r) { promise.set_value(r); }, runTempDir);

        if (fut.wait_for(hardDeadline) != std::future_status::ready) {
            driver->cancel();
            rec.status = "hard_timeout";
            rec.runtimeS = elapsedS();
            return rec;
        }
        LacamResults r = fut.get();
        rec.solved = r.solutionFound;
        rec.runtimeS = r.runtime;
        rec.solutionCost = r.solutionCost;
        rec.makespan = r.makespan;
        rec.socLowerBound = r.socLowerBound;
        rec.makespanLowerBound = r.makespanLowerBound;
        rec.status = r.solutionFound ? "solved" : (r.errorMessage.empty() ? "unsolved" : "error");
        if (!r.errorMessage.empty()) {
            std::cerr << "[run] " << prettyKey(task) << ": " << r.errorMessage << std::endl;
        }
    }

    return rec;
}

int ExperimentRunner::run(const std::function<void(const RunnerProgress&)>& onProgress)
{
    const std::vector<RunTask> allTasks = expandMatrix(m_spec);
    const std::vector<RunTask> tasks = pendingTasks();

    std::error_code ec;
    fs::create_directories(m_spec.outputDir, ec);
    if (ec) {
        std::cerr << "Error: cannot create output dir " << m_spec.outputDir << ": " << ec.message()
                  << std::endl;
        return 1;
    }

    // Refuse to double-append without --resume (silent duplicate rows would
    // corrupt downstream aggregation).
    if (!m_opts.resume && !ResultsCsv::readCompletedRows(resultsCsvPath()).empty()) {
        std::cerr << "Error: " << resultsCsvPath()
                  << " already contains results. Pass --resume to continue the "
                     "experiment, or remove/relocate the file."
                  << std::endl;
        return 2;
    }

    const std::string started = isoUtcNow();
    writeManifest(started, "", allTasks.size(), tasks.size());

    // Per-invocation temp root; per-run subdirectories inside.
    const std::string tempRoot =
        (fs::temp_directory_path() / ("openmapf-exp-" + std::to_string(::getpid()))).string();
    fs::create_directories(tempRoot, ec);

    std::mutex resultsMutex;
    RunnerProgress progress;
    progress.total = tasks.size();
    std::atomic<std::size_t> nextTask{0};
    std::atomic<bool> ioFailure{false};

    const int workerCount = static_cast<int>(std::min<std::size_t>(
        static_cast<std::size_t>(m_opts.jobs), std::max<std::size_t>(tasks.size(), 1)));

    auto worker = [&]() {
        while (!m_stop.load()) {
            const std::size_t idx = nextTask.fetch_add(1);
            if (idx >= tasks.size()) {
                break;
            }
            const RunTask& task = tasks[idx];

            const std::string runTempDir = tempRoot + "/run-" + std::to_string(idx);
            fs::create_directories(runTempDir);

            RunRecord rec = executeTask(task, runTempDir);

            // Per-run solver outputs are parsed into `rec`; clean them up.
            std::error_code cleanupEc;
            fs::remove_all(runTempDir, cleanupEc);

            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                if (!ResultsCsv::appendRow(resultsCsvPath(), rec)) {
                    std::cerr << "Error: failed to append to " << resultsCsvPath() << std::endl;
                    ioFailure.store(true);
                    m_stop.store(true);
                    return;
                }
                progress.completed++;
                progress.lastKey = prettyKey(task);
                progress.lastStatus = rec.status;
                if (onProgress) {
                    onProgress(progress);
                }
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(workerCount));
    for (int i = 0; i < workerCount; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    fs::remove_all(tempRoot, ec);
    writeManifest(started, isoUtcNow(), allTasks.size(), tasks.size());

    return ioFailure.load() ? 1 : 0;
}
