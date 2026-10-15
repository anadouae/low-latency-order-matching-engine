# Multi-stage build for the low-latency order matching engine.
FROM ubuntu:24.04 AS build

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    clang \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake --preset default -DOME_BUILD_BENCHMARKS=OFF \
 && cmake --build --preset default -j \
 && ctest --preset default --output-on-failure

FROM ubuntu:24.04 AS runtime
RUN useradd --create-home --uid 10001 ome
COPY --from=build /src/build/default/ome_cli /usr/local/bin/ome_cli
USER ome
ENTRYPOINT ["/usr/local/bin/ome_cli"]
CMD ["--demo"]
