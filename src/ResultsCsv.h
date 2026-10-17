#pragma once

#include <cstddef>
#include <string>
#include <vector>

// One run of one solver on one instance — a single row of the stable
// results schema documented in docs/file-formats.md. This is both the GUI
// "Export results" format and the batch experiment runner's output format,
// so downstream pandas/plot pipelines can rely on one schema.
struct RunRecord
{
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
    // Outcome category: "solved", "unsolved", "error", "hard_timeout"
    // (killed at the hard deadline), or "skipped" (instance unusable, e.g.
    // scen has fewer agents than requested).
    std::string status;
    // Lower bounds reported by the solver (LaCAM family); -1 = unknown.
    long socLowerBound = -1;
    long makespanLowerBound = -1;
};

namespace ResultsCsv {

// The exact header line (without trailing newline). Treat as a stable,
// versioned interface: append new columns only at the end (the 'status'
// column was appended this way).
extern const char* const kHeader;

// Number of columns in kHeader.
constexpr std::size_t kColumnCount = 18;

// Column indices that identify a run for resume purposes:
// solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s.
constexpr std::size_t kKeyColumnCount = 8;

// RFC-4180-style CSV escaping: fields containing comma, quote, or newline
// are quoted, with embedded quotes doubled.
std::string escapeField(const std::string& field);

// Format one record as a CSV row (without trailing newline).
std::string formatRow(const RunRecord& r);

// Split one CSV line into fields, honoring RFC-4180 quoting.
std::vector<std::string> splitRow(const std::string& line);

// Read all *complete* data rows from an existing results CSV: the header is
// skipped, a final line not terminated by a newline (crash during write) is
// ignored, and rows with fewer fields than the file's own header are
// ignored (robust to schema growth and to crash-repaired partial rows).
// Returns an empty vector when the file does not exist.
std::vector<std::vector<std::string>> readCompletedRows(const std::string& path);

// Append a record to the CSV at `path`: writes the header first when the
// file is missing or empty, appends the row, and flushes immediately so
// completed runs survive a crash (the batch runner's resume semantics
// depend on this). When the existing file has an older, narrower header,
// the row is cut to that header's width so the file stays rectangular.
// Returns false on I/O failure.
bool appendRow(const std::string& path, const RunRecord& r);

} // namespace ResultsCsv
