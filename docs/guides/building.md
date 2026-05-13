# Building OpenSRX {#building}

## Default Build (Shared Library)

```bash
cmake -S . -B build
cmake --build build
```

## CMake Options

| Option             | Description                              | Default |
|--------------------|------------------------------------------|---------|
| `BUILD_STATIC_LIB` | Build a static library instead of shared | `OFF`   |
| `BUILD_EXAMPLES`    | Build the example binaries               | `ON`    |
| `BUILD_TESTS`       | Build the test suite (fetches GoogleTest)| `ON`    |

Example – static library, no tests:

```bash
cmake -S . -B build -DBUILD_STATIC_LIB=ON -DBUILD_TESTS=OFF
cmake --build build
```

## Running the Tests

```bash
cmake -S . -B build
cmake --build build
./build/tests/TestOpenSRX
```

## Dependencies

All C++ dependencies are fetched automatically at configure time through CMake
`FetchContent`:

- **Asio** 1.30.2 – standalone, header-only networking
- **spdlog** 1.15.3 – fast C++ logging
- **fineftp-server** 1.3.4 – lightweight FTP server for image readback
- **GoogleTest** 1.17.0 – unit-testing framework (tests only)
- **argparse** 3.2 – CLI argument parsing (examples only)
