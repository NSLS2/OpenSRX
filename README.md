# OpenSRX [![CI](https://github.com/jwlodek/OpenSRX/actions/workflows/ci.yml/badge.svg)](https://github.com/jwlodek/OpenSRX/actions/workflows/ci.yml)

Basic CMake project scaffold with:

- a root `CMakeLists.txt`
- a shared library by default, or a static library when requested
- public headers under `include/`
- example binaries under `examples/`
- a placeholder `tests/` directory

## Build

```bash
cmake -S . -B build
cmake --build build
./build/examples/opensrx_example
```

## Options

```bash
cmake -S . -B build -DBUILD_STATIC_LIB=ON
cmake -S . -B build -DBUILD_TESTS=OFF
cmake -S . -B build -DBUILD_EXAMPLES=OFF
```

`BUILD_STATIC_LIB=OFF` builds the shared library. `BUILD_STATIC_LIB=ON` builds the static library instead.

## Test Dependencies

Tests use CMake `FetchContent` to download GoogleTest and GoogleMock version `1.17.0` during configure.

## Networking Dependency

The project uses standalone header-only Asio via CMake `FetchContent`, currently pinned to version `1.30.2`.

## Logging Dependency

The project uses `spdlog` via CMake `FetchContent`, currently pinned to version `1.15.3`.

## FTP Dependency

The project uses `fineftp-server` via CMake `FetchContent`, currently pinned to version `1.3.4`, for FTP-based image readback from the scanner.

## Documentation

The project uses [Doxygen](https://www.doxygen.nl/) to generate documentation
from both C++ header comments and hand-written Markdown guides.

### Build docs locally

```bash
# With pixi
pixi run docs

# Or manually
cd docs
doxygen Doxyfile
```

The generated site will be in `docs/_build/html/`.

The generated site will be in `docs/_build/html/`.
