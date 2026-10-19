# Bundled MovingAI benchmark data

The `.map` files in this directory come from the MovingAI pathfinding
benchmark collection, and the `.scen` files from the MovingAI MAPF benchmark
scenario suites:

- Maps: N. Sturtevant, "Benchmarks for Grid-Based Pathfinding",
  *IEEE Transactions on Computational Intelligence and AI in Games*, 2012.
  https://movingai.com/benchmarks/
- MAPF scenarios: R. Stern et al., "Multi-Agent Pathfinding: Definitions,
  Variants, and Benchmarks", *SoCS*, 2019.
  https://movingai.com/benchmarks/mapf.html

The benchmark data is made freely available for research use by its authors;
it is redistributed here unmodified, for convenience, as sample input data.
Please cite the papers above when using it.

The complete MovingAI scenario suites for every map in this directory are
included, unmodified:

| Folder | Content |
|---|---|
| `scen-random/` | `<map>-random-1.scen` to `<map>-random-25.scen` for each map (825 files) |
| `scen-even/` | `<map>-even-1.scen` to `<map>-even-25.scen` for each map (825 files) |

For convenience, a few of them are also copied next to the maps (unmodified
from the "random" suite); the examples and docs use these:

| File | For map | Entries |
|---|---|---|
| `empty-8-8-random-1.scen` | `empty-8-8.map` | 32 |
| `random-32-32-10-random-1.scen` | `random-32-32-10.map` | 461 |
| `room-32-32-4-random-1.scen` | `room-32-32-4.map` | 341 |
| `room-64-64-8-random-1.scen` | `room-64-64-8.map` | 1000 |
| `warehouse-10-20-10-2-1-random-1.scen` | `warehouse-10-20-10-2-1.map` | 1000 |

Load a scenario via *Map tab → Load Scenario* after loading the matching map.
A MAPF instance with k agents uses the first k entries of a scenario file, as
proposed by Stern et al. (2019).
