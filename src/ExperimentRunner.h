#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "ExperimentSpec.h"
#include "ResultsCsv.h"
#include "SolverManifest.h"

struct RunnerOptions {
    int jobs = 0;              // <= 0: defaultJobs()
    bool resume = false;       // skip runs already complete in results.csv
    int hardTimeoutGraceS = 10; // hard kill at time_limit_s + grace
    std::string toolVersion = "unknown";
};

struct RunnerProgress {
    std::size_t total = 0;     // pending tasks this invocation
    std::size_t completed = 0; // finished (row written)
    std::string lastKey;       // human-readable id of the last finished run
    std::string lastStatus;
};

// Headless batch experiment runner (plan addendum M2.5). Executes the
// expanded, shuffled run matrix with bounded parallel workers; appends one
// results-CSV row per run (flushed immediately); enforces a hard per-run
// timeout by killing the solver process; cleans per-run temp outputs after
// parsing; writes a provenance manifest.yaml next to the results.
class ExperimentRunner {
public:
    ExperimentRunner(ExperimentSpec spec, SolverManifest manifest, RunnerOptions opts);

    // Spec/manifest consistency errors (unknown solver key, unsupported
    // driver family, missing executable). Empty = OK to run.
    std::vector<std::string> validate() const;

    // The shuffled task list, minus completed runs when resuming.
    // (--dry-run prints from this.)
    std::vector<RunTask> pendingTasks() const;

    // Blocking; safe to call from a worker thread. Returns 0 when every
    // pending task was attempted (individual solver failures are recorded
    // in the CSV, not turned into a process failure), 1 on I/O errors that
    // prevent recording results, 2 when refusing to overwrite existing
    // results without --resume.
    int run(const std::function<void(const RunnerProgress&)>& onProgress = nullptr);

    // Stop picking new tasks and kill in-flight runs at the next check.
    void requestStop();

    std::string resultsCsvPath() const;
    std::string manifestPath() const;

    static int defaultJobs(); // min(4, hardware_concurrency()/2), >= 1

private:
    RunRecord executeTask(const RunTask& task, const std::string& runTempDir);
    void writeManifest(const std::string& started, const std::string& finished,
                       std::size_t totalTasks, std::size_t pendingCount) const;
    std::string solverVersion(const std::string& executable) const;

    ExperimentSpec m_spec;
    SolverManifest m_manifest;
    RunnerOptions m_opts;
    std::atomic<bool> m_stop{false};
};
