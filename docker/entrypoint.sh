#!/bin/sh
# Entry point of the OpenMAPF container. See docs/docker.md.
set -eu

ROOT=/opt/openmapf
WORK=/work

usage() {
    cat <<'EOF'
OpenMAPF container

  reproduce [run options]   run the bundled LaCAM benchmark (96 runs) and
                            write results.csv and figures to /work/benchmark
  smoke                     run the smoke experiment (fake solver, seconds)
  run <spec.yaml> [opts]    run your own experiment spec (paths inside the
                            container, e.g. under /work)
  plot <results.csv> [opts] make figures from a results file
  test                      run the unit and integration tests
  gui                       start the GUI (needs an X11 display, see docs)
  solvers                   list the bundled solvers
  shell                     open a shell
  help                      show this message

Mount a host directory at /work to keep the results:
  docker run --rm --user "$(id -u):$(id -g)" -v "$PWD/out:/work" openmapf reproduce
EOF
}

# Copy an example into /work, pointing its map paths at the bundled maps.
stage_example() {
    name=$1
    dest=$WORK/$2
    mkdir -p "$dest"
    sed "s#\.\./\.\./maps/#$ROOT/maps/#g" "$ROOT/examples/$name/experiment.yaml" >"$dest/experiment.yaml"
    for f in "$ROOT/examples/$name"/*; do
        case "$(basename "$f")" in
            experiment.yaml | out | README.md) ;;
            *) cp -r "$f" "$dest/" ;;
        esac
    done
}

cmd=${1:-help}
[ $# -gt 0 ] && shift

case "$cmd" in
    reproduce)
        stage_example benchmark-experiment benchmark
        openmapf run "$WORK/benchmark/experiment.yaml" "$@"
        openmapf-plot "$WORK/benchmark/out/results.csv" -o "$WORK/benchmark/out"
        openmapf-plot "$WORK/benchmark/out/results.csv" -o "$WORK/benchmark/out" --style paper \
            --maps "$ROOT/maps/movingai"
        echo "Results and figures: $WORK/benchmark/out (on the host: the directory mounted at /work)"
        ;;
    smoke)
        stage_example smoke-experiment smoke
        openmapf run "$WORK/smoke/experiment.yaml" --solvers "$WORK/smoke/solvers.yaml" "$@"
        openmapf-plot "$WORK/smoke/out/results.csv" -o "$WORK/smoke/out"
        ;;
    run) exec openmapf run "$@" ;;
    plot) exec openmapf-plot "$@" ;;
    test) exec "$ROOT/libexec/openmapf_tests" "$@" ;;
    gui) exec openmapf "$@" ;;
    solvers) exec cat "$ROOT/share/openmapf/solvers.yaml" ;;
    shell) exec /bin/bash "$@" ;;
    help | -h | --help) usage ;;
    openmapf | openmapf-plot) exec "$cmd" "$@" ;;
    *)
        echo "unknown command: $cmd" >&2
        usage >&2
        exit 2
        ;;
esac
