#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Per-series aggregates of a results CSV (docs/file-formats.md), for the
// GUI's live experiment plots. A series is one solver x grid combination,
// labeled like scripts/openmapf-plot does ("lacam3 (hex-oddr)"); its points
// are one per agent count. Rows with status "skipped" (no solver ran) are
// left out, as in openmapf-plot.
struct SummaryPoint
{
    int agents = 0;
    int runs = 0;   // attempted runs at this agent count
    int solved = 0; // of which solved
    // Means/sds over solved runs; NaN when nothing solved or not reported
    double meanCost = 0.0;
    double sdCost = 0.0;
    double meanRuntimeS = 0.0;
    double meanCostOverLb = 0.0; // NaN when the CSV has no positive soc_lb
    double sdCostOverLb = 0.0;

    double successRate() const { return runs > 0 ? static_cast<double>(solved) / runs : 0.0; }
};

struct SummarySeries
{
    std::string label;
    std::vector<SummaryPoint> points; // sorted by agents
};

struct ResultsSummary
{
    std::vector<SummarySeries> series; // sorted by label
    std::size_t rows = 0;              // non-skipped rows aggregated
    std::size_t solvedRows = 0;
    bool hasLowerBounds = false; // every solved row reported soc_lb > 0

    // header: the CSV's own header fields; columns are looked up by name
    static ResultsSummary fromRows(const std::vector<std::string>& header,
                                   const std::vector<std::vector<std::string>>& rows);

    // Reads a results CSV; an empty summary when the file does not exist.
    static ResultsSummary fromCsv(const std::string& path);
};
