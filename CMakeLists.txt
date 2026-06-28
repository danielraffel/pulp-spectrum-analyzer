cmake_minimum_required(VERSION 3.24)
project(pulp-spectrum-analyzer LANGUAGES CXX)

# Consume the Pulp SDK (provides pulp::signal/pulp::format, pulp_add_plugin,
# and a bundled Catch2). The simplest path is `pulp create` against a Pulp
# checkout/install; to build directly, point CMake at a Pulp build/install:
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
#         -DCMAKE_PREFIX_PATH=/path/to/pulp/install
find_package(Pulp CONFIG REQUIRED)

option(PULP_BUILD_TESTS "Build the analyzer tests" ON)
enable_testing()

# Layers 1-2 + persistent state: pure, headless-tested.
add_subdirectory(engine)

# Layer 4 (screenshot-testable spectrum panel) — Skia raster baselines.
add_subdirectory(ui)

# Layer 3 (RT publication) and the cross-instance registry / plugin shell land
# here as they are implemented — see README.md and ROADMAP.md.
