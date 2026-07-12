#pragma once

#include <string>

#include "ISolutionParser.h"

// Parses the CBSH2-RTC results CSV; the solver writes the standard arrow
// paths file itself (--outputPaths), so no conversion is needed.
class Cbsh2CsvParser : public ISolutionParser
{
public:
    void parse(const std::string& outputFile, const std::string& pathsFile,
               SolverOutcome& outcome) override;
};

// Parses the LaCAM 'key=value' log and converts its per-timestep solution
// section into the standard arrow paths format.
class LacamOutputParser : public ISolutionParser
{
public:
    explicit LacamOutputParser(std::string algorithmName)
        : m_algorithmName(std::move(algorithmName))
    {
    }

    void parse(const std::string& outputFile, const std::string& pathsFile,
               SolverOutcome& outcome) override;

private:
    std::string m_algorithmName;
};
