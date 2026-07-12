#include "ResultsCsv.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>

namespace ResultsCsv {

const char* const kHeader =
    "solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,"
    "solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version";

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
        << ',' << r.hlExpanded << ',' << r.llExpanded << ',' << escapeField(r.toolVersion);
    return row.str();
}

bool appendRow(const std::string& path, const RunRecord& r)
{
    std::error_code ec;
    const bool needHeader =
        !std::filesystem::exists(path, ec) || std::filesystem::file_size(path, ec) == 0;

    std::ofstream file(path, std::ios::app);
    if (!file.is_open()) {
        return false;
    }
    if (needHeader) {
        file << kHeader << '\n';
    }
    file << formatRow(r) << '\n';
    file.flush(); // completed rows must survive a crash (resume semantics)
    return file.good();
}

} // namespace ResultsCsv
