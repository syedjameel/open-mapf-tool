# OpenMAPF in a container: headless batch experiments, figures, and
# (optionally) the GUI over X11. See docs/docker.md.
#
#   docker build -t openmapf .
#   docker run --rm -v "$PWD/out:/work" openmapf reproduce
#
# Build arguments:
#   WITH_SOLVERS=1     bundle the prebuilt hexagonal LaCAM solvers (0 = none;
#                      the smoke experiment still works with its fake solver)
#   HEX_LACAM_URL      where to fetch the solver archive when it is not
#                      already in docker/solvers/
#   HEX_LACAM_SHA256   expected checksum of that archive (empty = not checked)

# ---------------------------------------------------------------------------
# build: compile, run the test suite, install into /opt/openmapf
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS build

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
        libglfw3-dev libyaml-cpp-dev libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN test -f vendor/imgui/imgui.h && test -f vendor/implot/implot.h || { \
        echo "vendor/ submodules are missing: run 'git submodule update --init' before docker build" >&2; \
        exit 1; }

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/openmapf \
    && cmake --build build --parallel \
    && ctest --test-dir build --output-on-failure \
    && cmake --install build

# ---------------------------------------------------------------------------
# solvers: unpack the prebuilt hexagonal LaCAM binaries
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS solvers

ARG WITH_SOLVERS=1
ARG HEX_LACAM_URL=https://github.com/syedjameel/open-mapf-tool/releases/latest/download/openmapf-hex-lacam-linux-x86_64.tar.gz
ARG HEX_LACAM_SHA256=
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY docker/solvers/ /tmp/solvers/
RUN mkdir -p /opt/openmapf-hex-lacam \
    && if [ "$WITH_SOLVERS" = "1" ]; then \
        archive=/tmp/solvers/openmapf-hex-lacam-linux-x86_64.tar.gz; \
        if [ ! -f "$archive" ]; then \
            echo "fetching $HEX_LACAM_URL"; \
            curl -fsSL -o "$archive" "$HEX_LACAM_URL"; \
        fi; \
        if [ -n "$HEX_LACAM_SHA256" ]; then \
            echo "$HEX_LACAM_SHA256  $archive" | sha256sum -c -; \
        fi; \
        tar -xzf "$archive" -C /opt --no-same-owner; \
        test -x /opt/openmapf-hex-lacam/lacam3/main; \
    fi

# ---------------------------------------------------------------------------
# runtime
# ---------------------------------------------------------------------------
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
        libglfw3 libyaml-cpp0.8 libopengl0 libgl1 libglx-mesa0 libgl1-mesa-dri \
        python3 python3-pandas python3-matplotlib \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /opt/openmapf /opt/openmapf
COPY --from=build /src/build/openmapf_tests /opt/openmapf/libexec/openmapf_tests
# the tests read their fixtures from the source path they were built with
COPY --from=build /src/tests/fixtures /src/tests/fixtures
COPY --from=solvers /opt/openmapf-hex-lacam /opt/openmapf-hex-lacam
COPY scripts/openmapf-plot /opt/openmapf/bin/openmapf-plot
COPY maps /opt/openmapf/maps
COPY examples /opt/openmapf/examples
COPY docker/solvers.yaml /opt/openmapf/share/openmapf/solvers.yaml
COPY docker/entrypoint.sh /opt/openmapf/bin/openmapf-docker

# Results land in /work (mount a host directory there). HOME points at a
# writable place so the GUI can keep its settings when run as any user.
ENV PATH=/opt/openmapf/bin:$PATH \
    HOME=/tmp \
    MPLBACKEND=Agg
WORKDIR /work
ENTRYPOINT ["openmapf-docker"]
CMD ["help"]
