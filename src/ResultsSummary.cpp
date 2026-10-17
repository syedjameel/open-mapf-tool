#include "ResultsSummary.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>

#include "ResultsCsv.h"

namespace {

struct Accumulator
{
    int runs = 0;
    std::vector<double> costs, runtimes, ratios;
};

double mean(const std::vector<double>& v)
{
    if (v.empty())
        return std::numeric_limits<double>::quiet_NaN();
    double s = 0.0;
    for (double x : v)
        s += x;
    return s / static_cast<double>(v.size());
}

// Sample standard deviation (n - 1), 0 for fewer than two values
double stddev(const std::vector<double>& v)
{
    if (v.size() < 2)
        return v.empty() ? std::numeric_limits<double>::quiet_NaN() : 0.0;
    const double m = mean(v);
    double s = 0.0;
    for (double x : v)
        s += (x - m) * (x - m);
    return std::sqrt(s / static_cast<double>(v.size() - 1));
}

double toDouble(const std::string& s, double fallback)
{
    try {
        std::size_t used = 0;
        const double v = std::stod(s, &used);
        return used > 0 ? v : fallback;
    } catch (...) {
        return fallback;
    }
}

} // namespace

ResultsSummary ResultsSummary::fromRows(const std::vector<std::string>& header,
                                        const std::vector<std::vector<std::string>>& rows)
{
    auto col = [&header](const char* name) -> int {
        auto it = std::find(header.begin(), header.end(), name);
        return it == header.end() ? -1 : static_cast<int>(it - header.begin());
    };
    const int cSolver = col("solver"), cAgents = col("agents"), cGrid = col("grid_type"),
              cLayout = col("hex_layout"), cSolved = col("solved"), cRuntime = col("runtime_s"),
              cCost = col("solution_cost"), cStatus = col("status"), cLb = col("soc_lb");

    ResultsSummary out;
    if (cSolver < 0 || cAgents < 0 || cSolved < 0) {
        return out;
    }
    auto field = [](const std::vector<std::string>& r, int c) -> std::string {
        return c >= 0 && c < static_cast<int>(r.size()) ? r[c] : std::string();
    };

    std::map<std::string, std::map<int, Accumulator>> acc;
    bool allLb = true;
    for (const auto& r : rows) {
        if (field(r, cStatus) == "skipped") {
            continue;
        }
        std::string grid = field(r, cGrid);
        if (grid == "hex" && !field(r, cLayout).empty()) {
            grid += "-" + field(r, cLayout);
        }
        const std::string label = field(r, cSolver) + " (" + grid + ")";
        const int agents = static_cast<int>(toDouble(field(r, cAgents), 0.0));
        Accumulator& a = acc[label][agents];
        a.runs++;
        out.rows++;
        if (field(r, cSolved) != "1") {
            continue;
        }
        out.solvedRows++;
        const double cost = toDouble(field(r, cCost), -1.0);
        if (cost >= 0) {
            a.costs.push_back(cost);
        }
        a.runtimes.push_back(toDouble(field(r, cRuntime), 0.0));
        const double lb = toDouble(field(r, cLb), -1.0);
        if (cost >= 0 && lb > 0) {
            a.ratios.push_back(cost / lb);
        } else {
            allLb = false;
        }
    }
    out.hasLowerBounds = out.solvedRows > 0 && allLb;

    for (const auto& [label, byAgents] : acc) {
        SummarySeries s;
        s.label = label;
        for (const auto& [agents, a] : byAgents) {
            SummaryPoint p;
            p.agents = agents;
            p.runs = a.runs;
            p.solved = static_cast<int>(a.runtimes.size());
            p.meanCost = mean(a.costs);
            p.sdCost = stddev(a.costs);
            p.meanRuntimeS = mean(a.runtimes);
            p.meanCostOverLb = mean(a.ratios);
            p.sdCostOverLb = stddev(a.ratios);
            s.points.push_back(p);
        }
        out.series.push_back(std::move(s));
    }
    return out;
}

ResultsSummary ResultsSummary::fromCsv(const std::string& path)
{
    std::ifstream in(path);
    std::string headerLine;
    if (!in.is_open() || !std::getline(in, headerLine)) {
        return {};
    }
    if (!headerLine.empty() && headerLine.back() == '\r') {
        headerLine.pop_back();
    }
    return fromRows(ResultsCsv::splitRow(headerLine), ResultsCsv::readCompletedRows(path));
}
