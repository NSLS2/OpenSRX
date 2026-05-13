# OpenSRX [![CI](https://github.com/jwlodek/OpenSRX/actions/workflows/ci.yml/badge.svg)](https://github.com/jwlodek/OpenSRX/actions/workflows/ci.yml)

A C++ library for interfacing with Keyence SR-X series barcode readers over
Ethernet (TCP socket) or RS-232 serial connections.

## Features

- **Scanner control** – reading, tuning, I/O terminal control, status queries,
  settings management (save/load/factory reset/backup)
- **Typed parameter access** – template-based get/set for bank, tuning,
  operation, and communication parameters with automatic type conversion
- **Structured read results** – `Code` struct with optional bounding box,
  center, code type, bank number, and angle metadata
- **Image readback** – embedded FTP server receives BMP images pushed by the
  scanner; decode, queue, and callback APIs
- **Two comm backends** – `SocketInterface` (TCP/IP) and `SerialInterface`
  (RS-232), both implementing `ICommInterface`
- **Simulator** – Python-based scanner simulator for development without hardware

## Quick Start

```cpp
#include "OpenSRX/OpenSRX.hpp"

// Connect over Ethernet
OpenSRX::SocketInterface comm("192.168.100.100", 9004);
OpenSRX::Scanner scanner(comm);

// Read a barcode
OpenSRX::Code code = scanner.startReading();
std::cout << "Read: " << code.data << std::endl;

// Get/set parameters
auto exposure = scanner.getParam<OpenSRX::BankParam::EXPOSURE_TIME>(1);
scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

Or with [pixi](https://pixi.sh):

```bash
pixi run build
```

## Examples

Six example programs are included under `examples/`:

| Example | Description |
|---------|-------------|
| `VersionInfo` | Print library and scanner version information |
| `ReadCode` | Trigger a barcode read and print the result |
| `ReadCodeDetailed` | Read with bounding box, center, and code type metadata |
| `ImageReadback` | Start FTP server, trigger a read, save the received image as BMP |
| `ImageSnapshot` | Take a camera snapshot (no decode) and save as BMP |
| `AdjustParams` | Read and modify scanner parameters |

All examples accept `--ip`/`--port` or `--serial` arguments. Run with `--help`
for details.

## Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_STATIC_LIB` | Build a static library instead of shared | `OFF` |
| `BUILD_EXAMPLES` | Build example binaries | `ON` |
| `BUILD_TESTS` | Build the test suite (fetches GoogleTest) | `ON` |

## Testing

```bash
cmake --build build
./build/tests/TestOpenSRX
```

## Dependencies

All C++ dependencies are fetched automatically via CMake `FetchContent`:

- **Asio** 1.30.2 – standalone, header-only networking
- **spdlog** 1.15.3 – fast C++ logging
- **fineftp-server** 1.3.4 – lightweight FTP server for image readback
- **GoogleTest** 1.17.0 – unit-testing framework (tests only)
- **argparse** 3.2 – CLI argument parsing (examples only)

## Simulator

A Python-based scanner simulator is provided for development and testing:

```bash
python scripts/simulator.py
# or: pixi run simulator
```

This opens a TCP server on port 9004 that emulates the SR-X command protocol,
including simulated barcode reads with generated images pushed via FTP.

## Documentation

API documentation is generated with [Doxygen](https://www.doxygen.nl/) from
the C++ header comments and hand-written Markdown guides.

```bash
pixi run docs
# or: cd docs && doxygen Doxyfile
```

The generated site will be in `docs/_build/html/`.
