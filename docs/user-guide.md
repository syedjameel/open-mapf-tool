# OpenMAPF user guide

Three parts: the [GUI](#gui), the [command line](#command-line-reference),
and the [configuration files](#configuration-reference). File format details
live in [file-formats.md](file-formats.md); batch experiments in
[experiments.md](experiments.md).

## GUI

```sh
build/openmapf [--config <env.yaml>]
```

The window has two dockable panels: the **Scene** (the grid canvas) and the
**Control Panel** with seven tabs. Animation controls appear at the bottom of
the Control Panel whenever paths are loaded.

### Scene navigation

| Action | Input |
|---|---|
| Pan | drag with the **right** mouse button, or tap-and-drag with the left button (touchpads) |
| Zoom | mouse wheel (clamped to 0.05×–10×; zoom centers on the cursor) |
| Edit cells / place agents | left click (only in the respective editing mode, see below) |

### Map tab

- **Load Map** — opens a `.map` file (MovingAI format; all bundled maps are
  under `maps/`). Loading a map resets tracked temporary files.
- **Load Scenario** — opens a MovingAI `.scen` file for the *current* map.
  A dialog shows the entry count, the map the scenario was made for (with a
  warning if its dimensions differ from the current grid), and an
  *"Agents (first k entries)"* slider. Loading replaces the placed agents;
  entries that are out of bounds, on blocked terrain, or reuse an already
  taken start/goal cell are skipped, with counts reported.
- **Remove Map** — clears the grid to all-free cells.
- **Clean Temp Files** — deletes the temp files tracked from solver runs.
- **Add/Remove Obstacles** — while checked, left-clicking a cell toggles it
  between free and obstacle. Cells occupied by agents cannot be toggled.
- **Add/Remove Agents** — while checked, the first left click places an
  agent's **start**, the second its **goal**. Clicking an existing agent's
  start removes the agent; clicking its goal clears just the goal. Blocked
  cells are refused. *Clear Agents* removes everything; *Save Scenario*
  writes the current map + agents as `.map`/`.scen` (choose between
  auto-cleaned temporary files or permanent files named after the map).

### Algorithm tab

Pick a solver (**CBS**, **LaCAM**, **LaCAM2**, **LaCAM3** — executables come
from the solver manifest, see below), optionally open **Set Parameters**
(CBS: time limit, heuristic, rectangle/corridor/target/mutex reasoning,
bypass, etc.; LaCAM: time limit, seed, verbosity), then **Solve**. A
progress dialog offers **Cancel**, which really kills the solver process.
On success the paths load automatically and the animation starts. If a
solver is not configured or its binary is missing, the run fails with a
message telling you which manifest key to set.

### Grid & Size tab

Grid type (**Square** or **Hexagonal** with the four offset layouts:
Odd-R, Even-R, Odd-Q, Even-Q) and grid dimensions (1–200 columns/rows).
Changing dimensions resets the grid content.

### Solvers tab

One row per manifest entry showing a status indicator — green `[found]`,
red `[missing]`, gray `[not configured]` — an editable executable path, a
**Browse...** file picker, and the supported grid tags. **Save solver
paths** writes the manifest to
`${XDG_CONFIG_HOME:-~/.config}/openmapf/solvers.yaml`, the
highest-precedence discovery location.

### Experiments tab

A thin front-end over the headless batch runner (the CLI is the primary
interface): **Load spec...** (`experiment.yaml`), shown matrix size and
output directory, a **Resume** checkbox, **Start**/**Stop**, a progress bar
with the last finished run, and the results CSV path on completion. *Stop*
finishes in-flight runs before stopping.

Below it, four live plots of the experiment's `results.csv` (also shown for
results that already exist when a spec is loaded): success rate, mean sum of
costs (±1 sd band), mean runtime (log scale) against the agent count, and
quality against runtime (sum of costs / lower bound when the solver reports
it). One line per solver × grid; the plots refresh as runs finish.

### Paths tab

- **Load Paths** — load a precomputed arrow-format path file (`.txt`,
  samples under `paths/`). Check **LACAM Format** *before* loading if the
  file stores `(column,row)` pairs; leave unchecked for CBS-produced files,
  which store `(row,column)` (see
  [file-formats.md](file-formats.md#coordinate-conventions-read-this-first)).
- **Remove Paths** / **Start Visualization**.

### Results tab

Statistics of the last solver run (solution cost, runtime, node
expansions). **Export last run** appends one row in the stable results-CSV
schema (including `tool_version` and `status`) to an editable CSV path.

### Animation controls (bottom of the Control Panel)

Speed slider (0.1×–5×), a **timeline slider** showing
`current step / max step` for scrubbing, **Show Agent Paths** (trails), and
**Reset Animation**.

## Command line reference

```
openmapf [options]                        launch the GUI
openmapf run <experiment.yaml> [options]  headless batch experiments
```

### GUI options

| Option | Meaning |
|---|---|
| `--config <path>` | use this `env.yaml`; a missing file is a hard error (exit 1) |
| `--version` | print `openmapf <version>` and exit 0 |
| `-h`, `--help` | usage, config discovery order, run options; exit 0 |

Unknown options exit 2. Without a display, the GUI exits 1 with a clear
GLFW error message.

### `run` options

| Option | Meaning |
|---|---|
| `--jobs <N>` | parallel workers (default `min(4, cores/2)`) |
| `--resume` | skip runs already complete in `results.csv` |
| `--dry-run` | print matrix size, the first shuffled runs, and solver-config notes; no execution |
| `--verbose` | also print each solver command line (default: one progress line per run) |
| `--solvers <path>` | solver manifest to use instead of discovery |
| `-h`, `--help` | run usage and exit codes |

Exit codes: **0** — every pending run was attempted (individual solver
failures are recorded as CSV rows, not process failures); **2** — spec or
solver-configuration errors, or refusing to append to an existing
`results.csv` without `--resume`; **1** — I/O failure while writing results.

### Environment variables

| Variable | Meaning |
|---|---|
| `OPENMAPF_CONFIG` | path to `env.yaml` (checked after `--config`) |
| `OPENMAPF_SOLVERS` | path to `solvers.yaml` (checked after `--solvers`) |
| `OPENMAPF_FRAME_LOG` | GUI only: write one CSV row per frame to this file (`t_s, frame_ms, work_ms, rows, cols, grid, agents, animating, time_step`; `work_ms` is the time before the buffer swap) |
| `OPENMAPF_VSYNC` | GUI only: `0` turns vsync off, so frames render as fast as possible (for performance measurements) |

## Configuration reference

### Discovery order

Both `env.yaml` and `solvers.yaml` are searched in the same locations,
first match wins:

1. the CLI flag (`--config` / `--solvers`)
2. `$OPENMAPF_CONFIG` / `$OPENMAPF_SOLVERS`
3. `${XDG_CONFIG_HOME:-$HOME/.config}/openmapf/<file>`
4. `<exe dir>/../share/openmapf/<file>` (installed layout)
5. `<exe dir>/../config/<file>` (build-tree layout)
6. `<exe dir>/config/<file>`

If no `env.yaml` is found, built-in defaults are used with a warning — the
app starts from any working directory either way.

### `env.yaml`

```yaml
params:
  width: 1920      # initial window width in pixels (values < 320 fall back to 1280)
  height: 720      # initial window height in pixels (values < 240 fall back to 720)
  square: true     # start in square-grid mode
  hexagon: false   # start in hex-grid mode
  rows: 12         # initial grid rows
  cols: 12         # initial grid columns
  ui_scale: 1.0    # font and widget scale for HiDPI screens; clamped to 0.5-4
```

Missing keys keep their defaults; a malformed file falls back to defaults
with a warning (never a crash). A legacy `solvers:` map
(`cbs`/`lacam`/`lacam2`/`lacam3` → executable path) is still honored but
`solvers.yaml` takes precedence.

### `solvers.yaml` (solver manifest)

```yaml
solvers:
  - key: cbs                     # stable id: cbs, lacam, lacam2, lacam3
    name: CBSH2-RTC-CHBP-HEX     # display name
    executable: /path/to/cbs     # empty = not configured; relative paths
                                 # resolve against this file's directory
    grids: [square, hex-oddr, hex-evenr, hex-oddq, hex-evenq]
```

Merge rules: entries with known keys update the defaults; an absent or
empty `executable` never erases a configured path (so template files are
safe); unknown keys are appended. Precedence:
`solvers.yaml` > legacy `env.yaml` `solvers:` keys > unset.
`scripts/get-solvers.sh` generates this file after building the reference
solvers; the GUI *Solvers* tab edits and saves it.
