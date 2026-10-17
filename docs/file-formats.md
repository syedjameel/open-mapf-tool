# OpenMAPF file formats

This document is the authoritative reference for every file format OpenMAPF
reads or writes. Line endings: all readers tolerate both LF and CRLF; all
writers emit LF.

## Coordinate conventions (read this first)

OpenMAPF's internal convention is `(x, y)` = `(column, row)` with the origin
at the **top-left**: `x` grows rightwards, `y` grows downwards. This matches
the MovingAI benchmark convention.

One historical exception exists and is preserved for compatibility:
**CBS arrow path files store each position as `(row, column)`** — the
reverse. When a paths file is loaded *without* the "LaCAM format" flag
(the default, used for CBSH2-RTC output), each `(a,b)` pair is interpreted
as `a` = row, `b` = column and swapped internally. Files produced by the
LaCAM conversion step are already `(column, row)` and are loaded with the
flag on (no swap). If you generate arrow files for OpenMAPF yourself,
either write `(row, column)` pairs, or write `(column, row)` and load them
with the LaCAM-format flag.

## MovingAI map (`.map`) — read and written

```
type octile
height <rows>
width <cols>
map
<rows lines of exactly <cols> terrain characters>
```

Terrain characters (per the MovingAI spec, movingai.com/benchmarks/formats.html):

| Char | Meaning | OpenMAPF cell value | Blocked? | Written back as |
|---|---|---|---|---|
| `.` | passable terrain | 0 (Free) | no | `.` |
| `G` | passable terrain | 0 (Free) | no | `.` |
| `@` | out of bounds | 1 (Obstacle) | yes | `@` |
| `T` | trees | 2 (Tree) | yes | `T` |
| `S` | swamp | 3 (Swamp) | yes | `S` |
| `W` | water | 4 (Water) | yes | `W` |
| `O` | out of bounds | 5 (OutOfBounds) | yes | `O` |
| anything else | unknown | 1 (Obstacle) | yes | `@` |

Notes:

- OpenMAPF is conservative for MAPF purposes: *every* non-plain-passable
  terrain code is blocked. (`S` is passable and `W` swimmable in the
  single-agent benchmarks; MAPF solvers treat maps as binary, and emitting
  them as free space would corrupt benchmark maps.)
- Terrain round-trips: loading a map and saving it re-emits the original
  codes (`G` normalizes to `.`).
- Rows shorter than `width` leave the missing cells at their previous value
  (Free for a fresh grid); when writing, cells outside the grid extent are
  emitted as `.`.

## MovingAI scenario (`.scen`, version 1) — read and written

```
version 1
<bucket> <map> <map-width> <map-height> <start-x> <start-y> <goal-x> <goal-y> <optimal-length>
```

- The `version` header line is optional on input (very old suites lack it);
  always written on output.
- Fields are whitespace-separated (the official suites use tabs; spaces are
  accepted). One line per agent.
- `start-x`/`goal-x` are **columns**, `start-y`/`goal-y` are **rows**
  (origin top-left) — the standard benchmark convention.
- `optimal-length` is the octile-metric optimal path length in the official
  suites. OpenMAPF ignores it on input and writes `0` on output.
- On input, malformed lines are skipped and counted (surfaced in the GUI).
- On output, `bucket` is the agent id and the real grid dimensions are
  written.

**Loading semantics** ("first k agents"): agents are taken from the top of
the file in order until `k` usable entries are found. An entry is skipped
(with per-reason counts reported) when its start or goal is out of bounds
for the current grid, lies on blocked terrain, or reuses a start (or goal)
cell already taken by an accepted agent. A scenario whose declared map
dimensions differ from the current grid raises a visible warning first.

The standard benchmark suites (Stern et al. 2019) load unmodified; two
sample files are bundled under `maps/movingai/` (see the README there).

## Arrow path format (`.txt`) — read and written

One line per agent:

```
Agent <id>: (a,b)->(a,b)->...->(a,b)
```

- Consecutive positions are one timestep apart; a repeated position means
  the agent waits.
- Coordinate order per pair depends on the producer — see *Coordinate
  conventions* above (CBS files: `(row,col)`; LaCAM-converted files:
  `(col,row)`).
- Lines without a `:` are ignored; a malformed coordinate aborts that
  agent's path at the last valid position.

## LaCAM output log — read (and converted)

Produced by LaCAM/LaCAM2/LaCAM3 via `-o`. `key=value` lines followed by an
optional solution section:

```
agents=3
map_file=example.map
solved=1
soc=12
soc_lb=10
makespan=2
makespan_lb=2
sum_of_loss=12
sum_of_loss_lb=10
comp_time=250
solution=
0:(0,0),(1,1),(2,2),
1:(1,0),(2,1),(2,3),
```

- `comp_time` is in **milliseconds** (converted to seconds internally).
- Each `solution=` line is `timestep:(x1,y1),(x2,y2),...` — the position of
  every agent at that timestep, `(column,row)`.
- OpenMAPF converts this to the arrow path format (one line per agent);
  agents missing at later timesteps are padded with their last position.
- A file with `solved=0` or without a `solution=` section yields metrics
  but no paths.

## CBS results CSV — read

CBSH2-RTC's `-o` output: one header line, then one row per (re)start. The
**last** valid row wins. Columns used by OpenMAPF (0-based):

| Index | Column | Used as |
|---|---|---|
| 0 | runtime | runtime (seconds) |
| 1 | #high-level expanded | high-level nodes |
| 3 | #low-level expanded | low-level nodes |
| 5 | solution cost | cost; `-1` = no solution |

Rows need at least 6 comma-separated fields; files with no valid row
(empty, header-only, truncated) are reported as parse failures.

## Results CSV — written (stable schema)

Written by the GUI's *Results → Export last run* and by the batch
experiment runner. **This schema is a stable interface**: new columns are
only ever appended.

```
solver,map,scenario,agents,grid_type,hex_layout,seed,time_limit_s,solved,runtime_s,solution_cost,makespan,hl_expanded,ll_expanded,tool_version,status,soc_lb,makespan_lb
```

| Column | Type | Meaning |
|---|---|---|
| solver | string | driver name (`CBS`, `LaCAM`, `LaCAM2`, `LaCAM3`, ...) |
| map | string | map name |
| scenario | string | scen file name, or `interactive` |
| agents | int | number of agents in the run |
| grid_type | string | `square` or `hex` |
| hex_layout | string | `oddr`/`evenr`/`oddq`/`evenq`, empty for square |
| seed | int | solver seed |
| time_limit_s | int | time limit handed to the solver |
| solved | int | `1` solved, `0` not solved |
| runtime_s | float | solver-reported runtime, seconds (6 decimals) |
| solution_cost | int | sum of costs; `-1` = unknown |
| makespan | int | makespan; `-1` = unknown |
| hl_expanded | int | high-level nodes expanded; `-1` = unknown |
| ll_expanded | int | low-level nodes expanded; `-1` = unknown |
| tool_version | string | OpenMAPF version that produced the row |
| status | string | `solved`, `unsolved`, `error`, `hard_timeout` (killed at the hard deadline), or `skipped` (instance unusable, e.g. too few scen entries) |
| soc_lb | int | solver-reported lower bound on the sum of costs; `-1` = unknown |
| makespan_lb | int | solver-reported lower bound on the makespan; `-1` = unknown |

Writer semantics: the header is written only when the file is new or empty;
appending to a file written by an older version keeps that file's columns
(rows are cut to its header width);
rows are appended and flushed immediately (a crashed batch run can be
resumed by counting complete rows). String fields containing commas,
quotes, or newlines are quoted RFC-4180 style with doubled quotes.
