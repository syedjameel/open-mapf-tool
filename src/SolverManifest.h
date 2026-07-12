#pragma once

#include <string>
#include <vector>

class Environment;

// One external solver known to the tool.
struct SolverEntry
{
    std::string key;                // stable identifier: "cbs", "lacam", "lacam2", "lacam3", ...
    std::string name;               // human-readable display name
    std::string executable;         // path to the solver binary; empty = not configured
    std::vector<std::string> grids; // supported grid tags (informational)
};

// The solver manifest (config/solvers.yaml): which external solver binaries
// exist and where. Discovered through the same location chain as env.yaml
// (ConfigLocator), user-editable from the GUI "Solvers" tab.
//
// File schema:
//   solvers:
//     - key: cbs
//       name: CBSH2-RTC-CHBP-HEX
//       executable: /path/to/cbs
//       grids: [square, hex-oddr, hex-evenr, hex-oddq, hex-evenq]
class SolverManifest
{
public:
    // The four solvers the tool ships drivers for, with unset executables.
    static SolverManifest withDefaults();

    // Merge entries from a solvers.yaml into this manifest. Known keys are
    // updated (an absent/empty 'executable' keeps the current value so a
    // template file never erases a configured path); unknown keys are
    // appended. Relative executable paths are resolved against the
    // manifest file's directory. Returns false (leaving the manifest
    // usable) on I/O or parse errors.
    bool loadFromFile(const std::string& path);

    // Write the manifest as solvers.yaml (creates parent directories).
    bool saveToFile(const std::string& path) const;

    // Legacy fallback: adopt executable paths from env.yaml's 'solvers'
    // section for entries that are still unconfigured.
    void applyLegacyEnvironment(const Environment& env);

    std::vector<SolverEntry>& entries() { return m_entries; }
    const std::vector<SolverEntry>& entries() const { return m_entries; }

    const SolverEntry* find(const std::string& key) const;
    SolverEntry* find(const std::string& key);

    // Executable path for a key, or "" when unknown/unconfigured.
    std::string executableFor(const std::string& key) const;

private:
    std::vector<SolverEntry> m_entries;
};
