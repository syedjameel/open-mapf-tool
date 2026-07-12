#pragma once

#include <string>

// Format-neutral outcome of one external solver run. Fields a given solver
// does not report keep their defaults (-1 / empty). Adapters map this onto
// the driver-specific result structs the GUI consumes.
struct SolverOutcome
{
    bool solved = false;
    double runtimeS = 0.0; // wall clock, unless the output reports better
    long cost = -1;        // sum of costs
    long makespan = -1;
    long hlExpanded = -1;
    long llExpanded = -1;
    long socLowerBound = -1;
    long makespanLowerBound = -1;
    long sumOfLoss = -1;
    long sumOfLossLowerBound = -1;
    long agents = 0;          // agents handed to the solver
    std::string pathsFile;    // standard arrow paths (empty if none)
    std::string errorMessage; // non-empty = the run failed
};

// Parses one solver's output files after its process exits successfully.
// Implementations are small and format-specific (plan 3.4: the documented
// extension point for solvers with exotic output).
class ISolutionParser
{
public:
    virtual ~ISolutionParser() = default;

    // outputFile: the file the solver was asked to write (-o).
    // pathsFile:  where standard arrow paths should end up; the parser
    //             either points outcome.pathsFile at an existing file or
    //             converts into pathsFile itself.
    // On failure, set outcome.errorMessage (and leave outcome.solved false).
    virtual void parse(const std::string& outputFile, const std::string& pathsFile,
                       SolverOutcome& outcome) = 0;
};
