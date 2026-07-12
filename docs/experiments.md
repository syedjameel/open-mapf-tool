# Batch experiments

OpenMAPF's batch runner executes a full experiment matrix — solvers ×
instances × grids × agent counts × seeds — headless (no display), in
parallel, resumably, and records one CSV row per run in the stable results
schema (see [file-formats.md](file-formats.md)). The GUI's *Experiments*
tab is a thin front-end over the same runner; the CLI is the primary
interface:

```sh
openmapf run experiment.yaml [--jobs N] [--resume] [--dry-run] [--solvers PATH]
```

## Quick start (no solvers required)

The bundled smoke experiment runs against a *fake* solver script, so it
works on a fresh clone before any real solver is installed:

```sh
build/openmapf run examples/smoke-experiment/experiment.yaml \
    --solvers examples/smoke-experiment/solvers.yaml
scripts/openmapf-plot examples/smoke-experiment/out/results.csv
```

24 runs execute in a few seconds; two PNG figures land next to the CSV.
For real experiments, install solvers (`scripts/get-solvers.sh` or the GUI
*Solvers* tab) and write a spec as below.

## Experiment spec (`experiment.yaml`)

```yaml
name: my-experiment        # required
output_dir: out            # optional; default experiments/<name>; relative to this file
time_limit_s: 30           # optional; per-run solver time limit (default 30)
shuffle_seed: 42           # optional; run-order shuffle seed (default 0)
seeds: [0, 1, 2]           # optional; list, or a count N meaning 0..N-1 (default [0])
agents: [5, 10, 15]        # required; list, or a ladder {from: 5, to: 50, step: 5}
solvers: [cbs, lacam2]     # required; solver-manifest keys (cbs, lacam, lacam2, lacam3)
grids: [square, hex-oddr]  # optional; square, hex-oddr, hex-evenr, hex-oddq, hex-evenq
instances:                 # required; paths relative to this file
  - {map: ../maps/movingai/empty-8-8.map, scen: ../maps/movingai/empty-8-8-random-1.scen}
```

The matrix is expanded in the order `solvers × instances × grids × agents ×
seeds`, then shuffled with `shuffle_seed` (deterministic Fisher–Yates, so
the order is identical on every machine) to spread heavy instances across
time. `--dry-run` prints the size and the first runs without executing.

Spec errors — missing fields, unknown grid tags, missing map/scen files,
solver keys without a configured executable — are reported with actionable
messages and exit code 2 before any run starts.

## Execution model

- **Workers**: `min(4, cores/2)` parallel runs by default; `--jobs N`
  overrides. Each run gets a private temp directory (under the system temp
  path) that is deleted after its output is parsed.
- **Agents**: each run takes the first *k* usable entries of the scen file
  (see file-formats.md for the skip rules). If the scenario cannot supply
  *k* agents the run is recorded as `skipped` and no solver starts.
- **Per-run timeout**: the solver gets `time_limit_s` as its own limit; if
  its process has still not exited `hard_timeout_grace_s` (default 10 s)
  later, it is killed and the row records `status=hard_timeout`.
- **Solver failures** (crash, non-zero exit, unparsable output) are
  recorded as `status=error` rows; the runner continues and still exits 0
  ("all runs attempted"). Exit 1 is reserved for I/O failures writing the
  results file.

## Results and resume semantics

All output lands in `output_dir`:

- **`results.csv`** — one row per run, appended and flushed immediately
  after each run finishes, header written once. Schema (stable, documented
  in file-formats.md):
  `solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version,status`
- **`manifest.yaml`** — provenance: verbatim spec copy, tool version,
  per-solver executable path and `git describe` of its checkout (the solver
  binary is never executed just to probe a version), shuffle seed, worker
  count, task counts, start/finish timestamps (rewritten at completion).

**Resume** (`--resume`): a run is skipped when a *complete* row with the
same key (the first 8 CSV columns) already exists. A final row truncated by
a crash (no trailing newline, or fewer fields than the file's header) does
not count and is re-run; the next append also repairs the file by
terminating the partial line. Without `--resume`, the runner refuses to
append to a non-empty `results.csv` (exit 2) so an accidental re-run can
never silently duplicate rows.

Changing the spec is safe with `--resume`: only runs whose keys are not in
the CSV yet are executed (e.g. extending the agent ladder or adding seeds).

## Plots

```sh
scripts/openmapf-plot <output_dir>/results.csv [-o plots/]
```

Needs Python 3 with pandas and matplotlib (no display required). Produces
`success_rate_vs_agents.png` (mean `solved` per agent count, one line per
solver × grid series) and `runtime_cactus.png` (solved runs sorted by
runtime, log scale). Any tool that reads CSV works too — the schema is
stable by contract.
