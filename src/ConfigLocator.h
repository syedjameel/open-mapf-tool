#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

// Robust discovery of the env.yaml configuration file, so the application
// can be started from any working directory. Resolution order:
//   1. --config <path> command-line argument
//   2. $OPENMAPF_CONFIG
//   3. ${XDG_CONFIG_HOME:-$HOME/.config}/openmapf/env.yaml
//   4. <exe dir>/../share/openmapf/env.yaml   (installed layout)
//   5. <exe dir>/../config/env.yaml           (build-tree layout: build/ next to config/)
//   6. <exe dir>/config/env.yaml
// If nothing is found the caller falls back to compiled-in defaults.
namespace ConfigLocator {

struct Inputs {
    std::string cliPath;       // value of --config, empty if not given
    std::string envPath;       // $OPENMAPF_CONFIG, empty if unset
    std::string xdgConfigHome; // $XDG_CONFIG_HOME, empty if unset
    std::string homeDir;       // $HOME, empty if unset
    std::string exeDir;        // directory containing the executable, empty if unknown
};

// Candidate config paths in priority order (pure function, unit-tested).
// `fileName` selects which file is being discovered within the same
// locations (e.g. "env.yaml", "solvers.yaml").
std::vector<std::string> candidatePaths(const Inputs& in,
                                        const std::string& fileName = "env.yaml");

// First candidate for which `exists` returns true, or nullopt.
std::optional<std::string> resolve(const Inputs& in,
                                   const std::function<bool(const std::string&)>& exists,
                                   const std::string& fileName = "env.yaml");

// ${XDG_CONFIG_HOME:-$HOME/.config}/openmapf — where user-edited config
// files are written (pure function; empty when neither variable is set).
std::string userConfigDir(const Inputs& in);

// Directory containing the running executable (via /proc/self/exe with an
// argv[0] fallback); empty string if it cannot be determined.
std::string executableDirectory(const char* argv0);

// Production wrappers: gather Inputs from the real process environment and
// check the filesystem.
Inputs systemInputs(const std::string& cliPath, const char* argv0);
std::optional<std::string> resolveFromSystem(const std::string& cliPath, const char* argv0,
                                             const std::string& fileName = "env.yaml");

} // namespace ConfigLocator
