# Contributing to OpenMAPF

Thanks for your interest! Bug reports, feature requests, solver adapters,
and pull requests are all welcome. Support is provided via GitHub Issues
(bugs) and GitHub Discussions (questions).

## Development build

```sh
git clone --recurse-submodules https://github.com/syedjameel/open-mapf-tool
cd open-mapf-tool
sudo apt install cmake libglfw3-dev libyaml-cpp-dev   # Ubuntu; or -DOPENMAPF_FETCH_DEPS=ON
cmake -B build -DOPENMAPF_WERROR=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Everything except the GUI itself runs headless — the unit tests, the batch
runner (`openmapf run`), and the bundled smoke experiment
(`examples/smoke-experiment/`, no real solvers needed) all work without a
display. Please run the suite and the smoke experiment before submitting.

## Code style

The tree is formatted with **clang-format 18** using the committed
`.clang-format` (LLVM base, 4-space indent). CI enforces it:

```sh
pip install clang-format==18.1.8
clang-format -i $(ls src/*.cpp src/*.h tests/*.cpp tests/*.h | grep -v imfilebrowser)
```

Vendored code (`vendor/`, `src/imfilebrowser.h`) is never reformatted.

## Tests

- New parsing/geometry/format logic needs unit tests (Catch2, `tests/`);
  put realistic input files under `tests/fixtures/`.
- Anything driving external processes should be tested with fake solver
  shell scripts (see `tests/test_solver_integration.cpp`) — CI has no real
  solvers installed.
- Builds must stay clean under `-Wall -Wextra` (CI runs `-Werror`) and the
  suite must pass under ASan/UBSan (`-DOPENMAPF_SANITIZE=ON`).

## Adding a solver

Solvers that already speak MovingAI `.map`/`.scen` and one of the known
output formats need **no code**: add an entry to `solvers.yaml` (see
`config/solvers.yaml` and docs/experiments.md).

For a solver with a new output format, the extension point is small and
deliberate:

1. Implement an `ISolutionParser` subclass (see `src/SolutionParsers.h`;
   existing parsers are ~50 lines) that turns the solver's output into a
   `SolverOutcome`.
2. Provide the command line via an `ExternalSolverDriver::InvocationBuilder`
   (see `CBSDriver.cpp` for the pattern).
3. Add fixture files from a real run under `tests/fixtures/` and a parser
   test.

Please open an issue first so we can agree on the manifest key.

## Pull requests

- Small, focused commits with conventional-style messages
  (`fix:`, `feat:`, `docs:`, `build:`, `test:`, ...).
- Include tests for behavior changes; update `docs/` when formats or CLI
  behavior change (the results-CSV schema is append-only!).
- CI (build matrix, tests, integration, format, sanitizers) must be green.
