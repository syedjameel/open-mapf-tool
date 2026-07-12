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

Bundled sample scenarios (unmodified from the "random" suite):

| File | For map | Entries |
|---|---|---|
| `empty-8-8-random-1.scen` | `empty-8-8.map` | 32 |
| `random-32-32-10-random-1.scen` | `random-32-32-10.map` | 461 |

Load them via *Map tab → Load Scenario* after loading the matching map, or
fetch the full suites from the URL above.
