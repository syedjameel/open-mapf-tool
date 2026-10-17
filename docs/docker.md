# Running OpenMAPF with Docker

The container image bundles the OpenMAPF binary, the prebuilt hexagonal
LaCAM, LaCAM2 and LaCAM3 solvers, the sample MovingAI maps, and the plotting
script. It is meant for reproducing experiments without installing a
toolchain; the GUI also runs from it on Linux hosts with X11.

## Build the image

```sh
git clone --recursive https://github.com/syedjameel/open-mapf-tool.git
cd open-mapf-tool
docker build -t openmapf .
```

The build compiles OpenMAPF, runs the full test suite (the build fails if a
test fails), and downloads the solver archive from the project's GitHub
release. To build offline, put `openmapf-hex-lacam-linux-x86_64.tar.gz` into
`docker/solvers/` first. To build without solvers (only the smoke experiment
then works), pass `--build-arg WITH_SOLVERS=0`.

## Reproduce the benchmark experiment

```sh
mkdir -p out
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD/out:/work" openmapf reproduce
```

This runs `examples/benchmark-experiment` (96 runs: LaCAM and LaCAM3, two
maps, square and hexagonal grids, 50 to 300 agents, two seeds; well under a
minute on a laptop) and writes to `out/benchmark/out/`:

- `results.csv`, one row per run (schema: [file-formats.md](file-formats.md))
- `manifest.yaml`, the provenance record (tool version, solvers, spec)
- `success_rate_vs_agents.png`, `runtime_cactus.png`,
  `solution_cost_vs_agents.png`
- `quality_vs_runtime.pdf` / `.png`, the publication-style figure

Run options are passed through, for example `reproduce --jobs 8`. If a run
is interrupted, start it again with `reproduce --resume`.

`--user "$(id -u):$(id -g)"` makes the output files belong to you rather
than to root.

## Other commands

| Command | What it does |
|---|---|
| `smoke` | 24-run smoke experiment with a fake solver (seconds) |
| `run <spec.yaml> [opts]` | your own experiment spec; put it under `/work` |
| `plot <results.csv> [opts]` | figures from a results file |
| `test` | the unit and integration tests |
| `solvers` | the bundled solver manifest |
| `gui` | the GUI over X11 (below) |
| `shell` | a shell inside the container |

Paths in your own specs are paths inside the container. The bundled maps are
under `/opt/openmapf/maps/movingai/`; your own files are wherever you mounted
them, for example `/work`:

```sh
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" openmapf run /work/my-experiment.yaml
```

The bundled solver manifest is used by default. To use other solvers, mount
their binaries and a manifest and pass `--solvers /work/solvers.yaml`.

## GUI (Linux, X11)

```sh
xhost +local:docker
docker run --rm -it --user "$(id -u):$(id -g)" \
    -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v "$PWD:/work" \
    openmapf gui
```

The container renders with Mesa's software OpenGL, which is fine for maps of
the benchmark size. For a smoother GUI, or on macOS and Windows, build
OpenMAPF natively (see the README). Settings the GUI saves live in the
container's `/tmp` and are lost when it exits.
