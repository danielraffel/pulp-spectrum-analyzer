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

# Layer 1: the pure analysis engine (built + tested today).
add_subdirectory(engine)

# Layers 2-4 (diff model, RT publication, GPU UI) and the cross-instance plugin
# shell land here as they are implemented — see README.md and ROADMAP.md.
