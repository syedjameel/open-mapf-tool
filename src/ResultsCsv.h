#pragma once

#include <cstddef>
#include <string>

// One run of one solver on one instance — a single row of the stable
// results schema documented in docs/file-formats.md. This is both the GUI
// "Export results" format and the batch experiment runner's output format,
// so downstream pandas/plot pipelines can rely on one schema.
struct RunRecord {
    std::string solver;     // e.g. "CBS", "LaCAM2"
    std::string map;        // map name or file
    std::string scenario;   // scen file name, or "interactive"
    std::size_t agents = 0; // number of agents in the run
    std::string gridType;   // "square" or "hex"
    std::string hexLayout;  // "", "oddr", "evenr", "oddq", "evenq"
    int seed = 0;
    int timeLimitS = 0;
    bool solved = false;
    double runtimeS = 0.0;
    long solutionCost = -1; // -1 = unknown / not applicable
    long makespan = -1;
    long hlExpanded = -1;
    long llExpanded = -1;
    std::string toolVersion; // OPENMAPF_VERSION of the producing binary
};

namespace ResultsCsv {

// The exact header line (without trailing newline). Treat as a stable,
// versioned interface: append new columns only at the end.
extern const char* const kHeader;

// RFC-4180-style CSV escaping: fields containing comma, quote, or newline
// are quoted, with embedded quotes doubled.
std::string escapeField(const std::string& field);

// Format one record as a CSV row (without trailing newline).
std::string formatRow(const RunRecord& r);

// Append a record to the CSV at `path`: writes the header first when the
// file is missing or empty, appends the row, and flushes immediately so
// completed runs survive a crash (the batch runner's resume semantics
// depend on this). Returns false on I/O failure.
bool appendRow(const std::string& path, const RunRecord& r);

} // namespace ResultsCsv
