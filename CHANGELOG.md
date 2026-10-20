# Changelog

All notable changes to OpenMAPF are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project
uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- The complete MovingAI MAPF scenario suites for every bundled map:
  `maps/movingai/scen-random/` and `maps/movingai/scen-even/` (25 each per
  map, unmodified), plus `room-64-64-8-random-1.scen` and
  `warehouse-10-20-10-2-1-random-1.scen` next to the maps.
- GUI startup options `--map`, `--grid` and `--plan` open a map, set the grid
  type and play a LaCAM solution without going through the menus.
- `OPENMAPF_FRAME_LOG=<file.csv>` writes per-frame GUI timings (frame time,
  time before the swap, grid size, agent count, animation state);
  `OPENMAPF_VSYNC=0` turns vsync off for such measurements.
- Docker image (`Dockerfile`, `docs/docker.md`): builds and tests OpenMAPF,
  bundles the prebuilt hexagonal LaCAM solvers, and runs experiments
  (`reproduce`, `smoke`, `run`), figures (`plot`), the tests, or the GUI
  over X11.
- `examples/benchmark-experiment`: a 96-run LaCAM/LaCAM3 benchmark on
  square and hexagonal grids, with the bundled
  `room-32-32-4-random-1.scen` scenario.
- `ui_scale` in `env.yaml` (0.5–4): scales fonts and widget sizes for HiDPI
  screens and screen recordings.
- `openmapf run --verbose`: prints each solver command line.
- `openmapf-plot` also writes `solution_cost_vs_agents.png`.
- Live result plots in the GUI Experiments tab (ImPlot): success rate, sum of
  costs, runtime, and quality vs runtime per solver x grid, refreshed as runs
  finish. ImPlot v1.0 is vendored as a submodule (MIT).
- `openmapf-plot --style paper`: quality-vs-runtime publication figure
  (per-map panels, map thumbnails, serif type, PDF + PNG).
- Results CSV: `soc_lb` and `makespan_lb` columns (solver-reported lower
  bounds) appended to the schema; appending to an older file keeps its
  narrower columns.

### Changed
- The default build type is Release when none is given, so a plain
  `cmake -B build` produces an optimized build.
- `openmapf run` prints one progress line per run; the solver command lines
  and path-conversion notes moved behind `--verbose`.

### Fixed
- The GUI aborted (or drew garbage in Release builds) when one window held
  more than 65,535 vertices, e.g. 400 agents with their paths on
  warehouse-10-20-10-2-1: Dear ImGui now uses 32-bit draw indices.
- On first start (no saved `imgui.ini`, always the case in Docker) the Scene
  window was hidden behind the Control Panel; they are now docked side by
  side.
- Loading a map, resizing the grid, or switching square/hex now re-frames
  the Scene and clears paths that belonged to the previous grid (a loaded
  map used to keep the previous grid's tile size and overflow the Scene).
- Result lines showed `?` for check/cross glyphs the built-in font lacks.
- `openmapf-plot` counted `skipped` runs (scenario too small for k agents)
  as failures in the success-rate figure.
- The Results tab only summarized CBS runs.
- `-DOPENMAPF_FETCH_DEPS=ON` failed to configure with CMake 4.

## [0.1.0] — unreleased

First publication-ready release, superseding the pre-hardening prototype
(kept as an internal baseline tag).

### Added
- **Batch experiment pipeline** (`openmapf run experiment.yaml`): headless,
  matrix expansion (solvers × instances × grids × agents × seeds),
  deterministic shuffled run order, bounded parallel workers, per-run
  hard-timeout kill, crash-safe append-only results CSV with
  resume-by-key, per-experiment provenance `manifest.yaml`; minimal GUI
  Experiments panel; `scripts/openmapf-plot` (success-rate and
  runtime-cactus figures); bundled no-solver smoke example
  (`examples/smoke-experiment/`).
- **MovingAI `.scen` loading** (Stern benchmark suites) with "first k
  agents" selection and per-reason skip reporting; two authentic sample
  scenario files bundled with attribution.
- **Results CSV export** with a stable, documented schema
  (incl. `tool_version` and `status`).
- **Solver manifest** (`solvers.yaml`): configure solver executables
  without recompiling; GUI Solvers tab with found/missing indicators and
  file picker; `scripts/get-solvers.sh` to fetch and build the reference
  solvers; discovery via CLI flag → env var → XDG → exe-relative paths.
- **CLI**: `--config`, `--version`, `--help`; `run` subcommand with
  `--jobs/--resume/--dry-run/--solvers` and documented exit codes.
- **Quality infrastructure**: 68 headless test cases (642 assertions,
  Catch2 via FetchContent) incl. fixture-based parser tests and
  fake-solver integration tests; GitHub Actions CI (Linux gcc/clang +
  best-effort macOS, integration steps, clang-format check, ASan/UBSan
  job); `install` target and TGZ/DEB packages (CPack); `-Wall -Wextra`
  clean with `OPENMAPF_WERROR`; `OPENMAPF_SANITIZE` option.
- **Docs**: user guide, batch-experiments guide, precise file-format
  grammars (incl. coordinate conventions), extending guide, releasing
  guide, rewritten README; CONTRIBUTING, Code of Conduct, issue/PR
  templates; third-party notices; this changelog; citation metadata.

### Fixed
- MovingAI terrain codes `T`/`S`/`W`/`O` were silently converted to free
  space on save, corrupting benchmark maps; all terrain now round-trips
  and blocks agents. CRLF maps no longer leak `\r` into the grid.
- Generated `.scen` files wrote hardcoded `64 64` dimensions.
- App crashed unless launched from `build/` (config was loaded from a
  relative path in a member initializer); config discovery is now robust
  and failures degrade to defaults with a warning.
- Cancelling a solver only set a flag while the process kept running;
  cancel now kills the child process. Solver commands no longer pass
  through a shell (user-typed map names could break/inject the command).
- Solver results were applied to GUI state from the solver thread (data
  race); results are now marshaled to the GUI thread.
- Temp-file cleanup could delete user files matching a pattern in the
  working directory; cleanup is now limited to tracked files in a
  dedicated system temp directory.
- CBS path auto-loading depended on an unrelated UI checkbox, silently
  transposing coordinates.
- Negative-coordinate clicks caused undefined behavior (float→size_t);
  zoom could go negative; unused `GL/glew.h` include broke builds on
  machines without GLEW headers.

### Changed
- Core logic (formats, drivers, config, experiments) split into a
  headless `openmapf_core` library, separate from the GUI executable.
- Both solver drivers unified over one `ExternalSolverDriver` engine with
  small per-format `ISolutionParser` classes — the documented extension
  point for new solvers.
- Hardcoded solver paths (`../algorithms/...`) replaced by the solver
  manifest; window size honors the config; version shown in the window
  title and `--version` (generated `Version.h`).
- Whole tree formatted with clang-format 18 (enforced in CI); dead code
  removed (unused render cache, path optimizer, deprecated hex
  conversion, unused config keys).
