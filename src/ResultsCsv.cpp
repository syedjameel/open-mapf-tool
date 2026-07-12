#include "ResultsCsv.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace ResultsCsv {

const char* const kHeader =
    "solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,"
    "solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version,status";

std::string escapeField(const std::string& field)
{
    if (field.find_first_of(",\"\n\r") == std::string::npos) {
        return field;
    }
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

std::string formatRow(const RunRecord& r)
{
    char runtime[32];
    std::snprintf(runtime, sizeof(runtime), "%.6f", r.runtimeS);

    std::ostringstream row;
    row << escapeField(r.solver) << ',' << escapeField(r.map) << ','
        << escapeField(r.scenario) << ',' << r.agents << ',' << escapeField(r.gridType) << ','
        << escapeField(r.hexLayout) << ',' << r.seed << ',' << r.timeLimitS << ','
        << (r.solved ? 1 : 0) << ',' << runtime << ',' << r.solutionCost << ',' << r.makespan
        << ',' << r.hlExpanded << ',' << r.llExpanded << ',' << escapeField(r.toolVersion)
        << ',' << escapeField(r.status);
    return row.str();
}

std::vector<std::string> splitRow(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

std::vector<std::vector<std::string>> readCompletedRows(const std::string& path)
{
    std::vector<std::vector<std::string>> rows;

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return rows;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string content = ss.str();

    // A row is only complete once its newline hit the disk; drop anything
    // after the final newline (a row truncated by a crash).
    std::size_t lastNewline = content.rfind('\n');
    if (lastNewline == std::string::npos) {
        return rows;
    }
    content.resize(lastNewline);

    std::istringstream lines(content);
    std::string line;
    bool first = true;
    std::size_t expectedFields = kKeyColumnCount;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (first) {
            first = false;
            // Completeness is judged against this file's own header width,
            // so resuming works across schema versions and crash-repaired
            // partial rows are never mistaken for complete ones.
            expectedFields = std::max(kKeyColumnCount, splitRow(line).size());
            continue;
        }
        if (line.empty()) {
            continue;
        }
        auto fields = splitRow(line);
        if (fields.size() >= expectedFields) {
            rows.push_back(std::move(fields));
        }
    }
    return rows;
}

bool appendRow(const std::string& path, const RunRecord& r)
{
    std::error_code ec;
    const bool needHeader =
        !std::filesystem::exists(path, ec) || std::filesystem::file_size(path, ec) == 0;

    // Crash repair: if the previous process died mid-row the file ends
    // without a newline; terminate that partial row so the new row does not
    // concatenate onto it (readCompletedRows ignores the partial row either
    // way).
    bool needRepairNewline = false;
    if (!needHeader) {
        std::ifstream check(path, std::ios::binary | std::ios::ate);
        if (check.is_open() && check.tellg() > 0) {
            check.seekg(-1, std::ios::end);
            char last = '\n';
            check.get(last);
            needRepairNewline = last != '\n';
        }
    }

    std::ofstream file(path, std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    if (needHeader) {
        file << kHeader << '\n';
    } else if (needRepairNewline) {
        file << '\n';
    }
    file << formatRow(r) << '\n';
    file.flush(); // completed rows must survive a crash (resume semantics)
    return file.good();
}

} // namespace ResultsCsv
