# Smoke experiment (no solvers required)

Exercises the whole batch pipeline -- spec expansion, parallel workers,
results CSV, provenance manifest, plots -- using a bundled *fake* solver
script and the bundled MovingAI sample data.

From the repository root, after building:

```sh
build/openmapf run examples/smoke-experiment/experiment.yaml \
    --solvers examples/smoke-experiment/solvers.yaml
scripts/openmapf-plot examples/smoke-experiment/out/results.csv
```

Expected: 24 runs (1 solver x 2 instances x 2 grids x 3 agent counts x
2 seeds), all `solved`, results under `examples/smoke-experiment/out/`.
See docs/experiments.md for the spec grammar and resume semantics.
