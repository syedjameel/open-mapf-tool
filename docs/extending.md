# Extending OpenMAPF

Three extension scenarios, in increasing order of effort:

1. [Use your own build of a supported solver family](#1-point-a-solver-key-at-your-own-binary) — configuration only.
2. [Integrate a solver with a new output format](#2-add-a-solver-with-a-new-output-format) — a small, well-bounded C++ change.
3. [New grid geometries](#3-future-new-grid-geometries) — planned refactor; read this before attempting.

## 1. Point a solver key at your own binary

**No code needed** if your solver speaks the same command line and output
format as one of the four built-in driver families:

| Manifest key | Command-line contract | Output contract |
|---|---|---|
| `cbs` | CBSH2-RTC style: `-m <map> -a <scen> -o <csv> -k <n> -t <s> --seed=... --outputPaths=<txt>` (plus tuning flags; `--hexGrid=true --gridConfig=<layout>` only on hexagonal grids, so the upstream solver runs unchanged on square grids) | results CSV + arrow paths file ([file-formats.md](file-formats.md)) |
| `lacam`, `lacam2`, `lacam3` | LaCAM style: `-m <map> -i <scen> -N <n> -t <s> -s <seed> -v <lvl> -o <log>` (+ `-g square|hex -hc <layout>`) | `key=value` log with `solution=` section |

Worked example — you built a CBSH2-RTC fork at `~/solvers/mycbs/cbs`:

```yaml
# ~/.config/openmapf/solvers.yaml   (highest-precedence discovery location)
solvers:
  - key: cbs
    name: My CBSH2-RTC fork
    executable: /home/you/solvers/mycbs/cbs
    grids: [square]
```

Check it end to end without running anything:

```sh
openmapf --help                          # shows the discovery order
openmapf run my-experiment.yaml --dry-run   # prints matrix + solver-config notes
```

Then run the GUI (*Solvers* tab should show `[found]`) or
`openmapf run my-experiment.yaml`. Relative `executable` paths resolve
against the manifest file, so a manifest can ship next to its binaries.
The bundled `examples/smoke-experiment/` uses exactly this mechanism to
substitute a fake shell script for a real solver — it is the reference for
testing an integration without solving anything.

## 2. Add a solver with a new output format

The extension seam is deliberate and small (see `src/ExternalSolverDriver.h`):
every driver is the shared engine plus two solver-specific parts —

- an **`ExternalSolverDriver::InvocationBuilder`**: given the generated
  `mapFile`/`scenFile`, a unique `prefixPath`, and the agent count, return
  the `argv` plus which output/paths files the run uses;
- an **`ISolutionParser`** (`src/ISolutionParser.h`): after the process
  exits 0, turn its output files into a format-neutral `SolverOutcome`
  (solved flag, metrics, paths file, or an error message).

The two existing parsers (`src/SolutionParsers.{h,cpp}`) are ~50 lines each
and are the best templates. Steps:

1. **Parser**: subclass `ISolutionParser`; set `outcome.errorMessage` on
   parse failure, `outcome.solved`/metrics/`outcome.pathsFile` on success.
   If your solver does not emit arrow-format paths itself, convert into the
   provided `pathsFile` (see `LacamOutputParser`).
2. **Driver adapter**: follow `CBSDriver` (`src/CBSDriver.{h,cpp}`) — a thin
   class holding an `ExternalSolverDriver m_engine` (constructed with a
   display name, a manifest key, and an example executable path for error
   hints), building the invocation, and mapping `SolverOutcome` onto your
   results struct.
3. **Register the key** everywhere it is dispatched (grep for `"lacam3"` to
   find all sites): the manifest defaults
   (`SolverManifest::withDefaults`), the batch runner
   (`kKnownDriverKeys` + the driver dispatch in
   `ExperimentRunner::executeTask`), and — if you want GUI support — the
   algorithm list in `UIManager`.
4. **Tests** (CI has no real solvers): put output files from a real run
   under `tests/fixtures/` and test the parser against them (see
   `tests/test_solver_output_parsers.cpp`, including a `solved=0` and a
   truncated fixture), plus a fake-solver shell-script integration test
   (see `tests/test_solver_integration.cpp`).

Please open an issue first so the manifest key can be agreed on.

## 3. Future: new grid geometries

Honest status: square-vs-hex is **not** yet behind an abstraction. It is a
pair of flags (`Environment::m_square` / `m_hexagon`) plus a `HexGridType`
enum, with per-layout `switch` blocks in `GridRenderer::drawHexGrid`,
`GridRenderer::gridToWorld`, `GridRenderer::pixelToHex`, and the drivers'
grid-config argument mapping. Adding, say, triangular grids today would
mean touching all of those sites.

The planned refactor (a `GridGeometry` interface with
`cellToWorld`/`worldToCell`/`cellOutline`/`neighbors`/`solverConfigTag`
implementations per topology, replacing the flags with a single polymorphic
object) is planned for a later release. If you need a new topology
sooner, open an issue — coordinating with that refactor will save you a
rewrite. The coordinate-geometry unit tests in `tests/` are the safety net
any such change must keep green.
