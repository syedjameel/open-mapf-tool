# Benchmark experiment

A small but real benchmark: LaCAM and LaCAM3 on `random-32-32-10` and
`room-32-32-4`, on square and hexagonal (odd-r) grids, with 50 to 300 agents
and two seeds: 96 runs of at most 2 s each.

With Docker (bundles the solvers):

```sh
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD/out:/work" openmapf reproduce
```

From a local build, with the hexagonal LaCAM solvers in your solver manifest:

```sh
build/openmapf run examples/benchmark-experiment/experiment.yaml --jobs 4
scripts/openmapf-plot examples/benchmark-experiment/out/results.csv
scripts/openmapf-plot examples/benchmark-experiment/out/results.csv --style paper
```

Results go to `examples/benchmark-experiment/out/`: `results.csv`, the
provenance `manifest.yaml`, and the figures.
