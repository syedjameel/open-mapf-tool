#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "App.h"
#include "ConfigLocator.h"
#include "ExperimentRunner.h"
#include "ExperimentSpec.h"
#include "SolverManifest.h"
#include "Version.h"

namespace {

void printUsage(const char* argv0)
{
    std::cout << "OpenMAPF " << OPENMAPF_VERSION
              << " - interactive Multi-Agent Path Finding visualizer\n\n"
              << "Usage: " << argv0 << " [options]                 launch the GUI\n"
              << "       " << argv0 << " run <experiment.yaml> [run options]\n"
              << "                                          headless batch experiments\n\n"
              << "Options:\n"
              << "  --config <path>   Use this config file (YAML)\n"
              << "  --version         Print version and exit\n"
              << "  -h, --help        Print this help and exit\n\n"
              << "Run options ('" << argv0 << " run --help' for details):\n"
              << "  --jobs <N>        Parallel workers (default: min(4, cores/2))\n"
              << "  --resume          Skip runs already complete in results.csv\n"
              << "  --dry-run         Print the expanded run matrix and exit\n"
              << "  --solvers <path>  Use this solver manifest instead of discovery\n\n"
              << "Config discovery order (first match wins):\n"
              << "  1. --config <path>\n"
              << "  2. $OPENMAPF_CONFIG\n"
              << "  3. ${XDG_CONFIG_HOME:-$HOME/.config}/openmapf/env.yaml\n"
              << "  4. <exe dir>/../share/openmapf/env.yaml\n"
              << "  5. <exe dir>/../config/env.yaml\n"
              << "  6. <exe dir>/config/env.yaml\n"
              << "If no config is found, built-in defaults are used.\n\n"
              << "The solver manifest (solvers.yaml) is discovered through the same\n"
              << "locations (override with $OPENMAPF_SOLVERS). It lists the external\n"
              << "solver executables; solvers without a configured executable are\n"
              << "shown as unavailable in the GUI.\n";
}

void printRunUsage(const char* argv0)
{
    std::cout << "Usage: " << argv0 << " run <experiment.yaml> [options]\n\n"
              << "Runs the experiment matrix (solvers x instances x grids x agents x\n"
              << "seeds) headless and appends one row per run to\n"
              << "<output_dir>/results.csv (schema: docs/file-formats.md), plus a\n"
              << "provenance manifest.yaml. See docs/experiments.md.\n\n"
              << "Options:\n"
              << "  --jobs <N>        Parallel workers (default: min(4, cores/2))\n"
              << "  --resume          Skip runs already complete in results.csv\n"
              << "  --dry-run         Print matrix size and the first rows, then exit\n"
              << "  --solvers <path>  Solver manifest to use (default: discovered\n"
              << "                    solvers.yaml; $OPENMAPF_SOLVERS also works)\n"
              << "  -h, --help        Print this help and exit\n\n"
              << "Exit codes: 0 = all pending runs attempted; 2 = spec/solver-config\n"
              << "error or refusing to overwrite existing results without --resume;\n"
              << "1 = I/O failure while recording results.\n";
}

int runCommand(int argc, char** argv)
{
    std::string specPath;
    std::string solversOverride;
    RunnerOptions opts;
    opts.toolVersion = OPENMAPF_VERSION;
    bool dryRun = false;

    for (int i = 2; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            printRunUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(arg, "--resume") == 0) {
            opts.resume = true;
            continue;
        }
        if (std::strcmp(arg, "--dry-run") == 0) {
            dryRun = true;
            continue;
        }
        if (std::strcmp(arg, "--jobs") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --jobs requires a number" << std::endl;
                return 2;
            }
            opts.jobs = std::atoi(argv[++i]);
            if (opts.jobs <= 0) {
                std::cerr << "Error: --jobs must be >= 1" << std::endl;
                return 2;
            }
            continue;
        }
        if (std::strcmp(arg, "--solvers") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --solvers requires a path" << std::endl;
                return 2;
            }
            solversOverride = argv[++i];
            continue;
        }
        if (arg[0] == '-') {
            std::cerr << "Error: unknown run option '" << arg << "'\n" << std::endl;
            printRunUsage(argv[0]);
            return 2;
        }
        if (!specPath.empty()) {
            std::cerr << "Error: multiple spec files given" << std::endl;
            return 2;
        }
        specPath = arg;
    }

    if (specPath.empty()) {
        std::cerr << "Error: no experiment spec given\n" << std::endl;
        printRunUsage(argv[0]);
        return 2;
    }

    // --- spec ---
    ExperimentSpec spec;
    try {
        spec = ExperimentSpec::loadFromFile(specPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }

    // --- solver manifest ---
    SolverManifest manifest = SolverManifest::withDefaults();
    if (!solversOverride.empty()) {
        if (!std::filesystem::is_regular_file(solversOverride)) {
            std::cerr << "Error: solver manifest not found: " << solversOverride << std::endl;
            return 2;
        }
        manifest.loadFromFile(solversOverride);
        std::cout << "Using solver manifest: " << solversOverride << std::endl;
    } else {
        auto in = ConfigLocator::systemInputs("", argv[0]);
        const char* env = std::getenv("OPENMAPF_SOLVERS");
        in.envPath = env != nullptr ? env : "";
        auto discovered = ConfigLocator::resolve(
            in,
            [](const std::string& p) {
                std::error_code ec;
                return std::filesystem::is_regular_file(p, ec);
            },
            "solvers.yaml");
        if (discovered) {
            manifest.loadFromFile(*discovered);
            std::cout << "Using solver manifest: " << *discovered << std::endl;
        }
    }

    ExperimentRunner runner(std::move(spec), std::move(manifest), opts);

    if (dryRun) {
        const auto pending = runner.pendingTasks();
        std::cout << "Experiment matrix: " << pending.size() << " runs"
                  << (opts.resume ? " pending (after resume filtering)" : "") << "\n"
                  << "Results: " << runner.resultsCsvPath() << "\n"
                  << "First runs (shuffled order):\n";
        const std::size_t show = std::min<std::size_t>(pending.size(), 10);
        for (std::size_t i = 0; i < show; ++i) {
            const auto& t = pending[i];
            std::cout << "  " << t.solver << "  " << t.mapName() << "  " << t.scenName() << "  "
                      << t.grid << "  k=" << t.agents << "  seed=" << t.seed << "\n";
        }
        if (pending.size() > show) {
            std::cout << "  ... " << (pending.size() - show) << " more\n";
        }
        // Report (but do not fail on) solver-config problems in dry runs
        for (const auto& err : runner.validate()) {
            std::cout << "note: " << err << "\n";
        }
        return 0;
    }

    const auto errors = runner.validate();
    if (!errors.empty()) {
        for (const auto& err : errors) {
            std::cerr << "Error: " << err << std::endl;
        }
        return 2;
    }

    std::cout << "Running " << runner.pendingTasks().size() << " runs with "
              << "jobs=" << (opts.jobs > 0 ? opts.jobs : ExperimentRunner::defaultJobs()) << " -> "
              << runner.resultsCsvPath() << std::endl;

    int rc = runner.run([](const RunnerProgress& p) {
        std::cout << "[" << p.completed << "/" << p.total << "] " << p.lastKey << ": "
                  << p.lastStatus << std::endl;
    });

    if (rc == 0) {
        std::cout << "Done. Results: " << runner.resultsCsvPath() << std::endl;
    }
    return rc;
}

} // namespace

int main(int argc, char** argv)
{
    // Headless subcommand: never touches GLFW/OpenGL
    if (argc >= 2 && std::strcmp(argv[1], "run") == 0) {
        return runCommand(argc, argv);
    }

    std::string cliConfig;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strcmp(arg, "--version") == 0) {
            std::cout << "openmapf " << OPENMAPF_VERSION << std::endl;
            return 0;
        }
        if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(arg, "--config") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: --config requires a path argument" << std::endl;
                return 2;
            }
            cliConfig = argv[++i];
            continue;
        }
        std::cerr << "Error: unknown argument '" << arg << "'\n" << std::endl;
        printUsage(argv[0]);
        return 2;
    }

    // An explicitly requested config that does not exist is a hard error;
    // silently falling back would hide typos.
    if (!cliConfig.empty() && !std::filesystem::is_regular_file(cliConfig)) {
        std::cerr << "Error: config file not found: " << cliConfig << std::endl;
        return 1;
    }

    auto configPath = ConfigLocator::resolveFromSystem(cliConfig, argv[0]);
    if (configPath) {
        std::cout << "Using config: " << *configPath << std::endl;
    } else {
        std::cerr << "Warning: no config file found; using built-in defaults "
                  << "(see --help for the search order)." << std::endl;
    }

    // Solver manifest: same discovery chain, $OPENMAPF_SOLVERS override
    auto solversInputs = ConfigLocator::systemInputs("", argv[0]);
    const char* solversEnv = std::getenv("OPENMAPF_SOLVERS");
    solversInputs.envPath = solversEnv != nullptr ? solversEnv : "";
    auto solversPath = ConfigLocator::resolve(
        solversInputs,
        [](const std::string& p) {
            std::error_code ec;
            return std::filesystem::is_regular_file(p, ec);
        },
        "solvers.yaml");
    if (solversPath) {
        std::cout << "Using solver manifest: " << *solversPath << std::endl;
    }

    ImGuiMapfApp app(configPath.value_or(""), solversPath.value_or(""));
    return app.run();
}
