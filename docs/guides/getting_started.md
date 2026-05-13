# Getting Started {#getting_started}

OpenSRX is a C++ library that lets you control Keyence SR-X series barcode
readers programmatically. It supports two communication backends:

- **SocketInterface** – TCP/IP over Ethernet (default port 9004)
- **SerialInterface** – RS-232 serial link

## Prerequisites

- A C++17 compiler (GCC ≥ 9, Clang ≥ 10, MSVC ≥ 19.20)
- CMake ≥ 3.16
- An internet connection during the first build (dependencies are fetched
  automatically via CMake `FetchContent`)

## Quick Start

```bash
git clone https://github.com/jwlodek/OpenSRX.git
cd OpenSRX
cmake -S . -B build
cmake --build build
```

After building, example binaries are at `build/examples/` (e.g. `VersionInfo`,
`ReadCode`, `ImageReadback`). All examples accept `--ip`/`--port` or `--serial`
arguments — run with `--help` for usage details.

## Umbrella Header

Include a single header to get the entire public API:

```cpp
#include "OpenSRX/OpenSRX.hpp"
```

## Connecting to a Scanner

```cpp
#include "OpenSRX/OpenSRX.hpp"

// Over Ethernet
OpenSRX::SocketInterface comm("192.168.100.100", 9004);
OpenSRX::Scanner scanner(comm);

std::cout << scanner.getModel() << std::endl;          // e.g. "SR-X300"
std::cout << scanner.getFirmwareVersion() << std::endl; // e.g. "R2.04.00"
```

## Reading a Barcode

`startReading()` blocks until the scanner decodes a code (or throws on timeout):

```cpp
OpenSRX::Code code = scanner.startReading();
std::cout << "Data: " << code.data << std::endl;
```

Enable appending options for extra metadata:

```cpp
scanner.setParam<OpenSRX::OperationParam::CODE_VERTEX_APPENDING>(OpenSRX::Toggle::ENABLE);
scanner.setParam<OpenSRX::OperationParam::CODE_CENTER_APPENDING>(OpenSRX::Toggle::ENABLE);

OpenSRX::Code code = scanner.startReading();
if (code.boundingBox) {
    auto& bb = *code.boundingBox;
    std::cout << "Top-left: (" << bb.topLeft.x << ", " << bb.topLeft.y << ")\n";
}
if (code.center) {
    std::cout << "Center: (" << code.center->x << ", " << code.center->y << ")\n";
}
```

## Getting and Setting Parameters

Parameters are accessed with compile-time-typed templates:

```cpp
// Bank parameters (require a bank number)
int exposure = scanner.getParam<OpenSRX::BankParam::EXPOSURE_TIME>(1);
scanner.setParam<OpenSRX::BankParam::EXPOSURE_TIME>(1, 500);

// Operation parameters
auto fmt = scanner.getParam<OpenSRX::OperationParam::IMAGE_FORMAT>();
scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);

// Communication parameters
std::string ip = scanner.getParam<OpenSRX::CommParam::FTP_REMOTE_IP>();
```

## Image Readback

Start an embedded FTP server, trigger a read, and receive the image:

```cpp
scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);
scanner.startImageServer("192.168.100.50");  // your machine's IP

OpenSRX::Code code = scanner.startReading();
OpenSRX::Image img = scanner.waitForImage();
// img.width, img.height, img.channels, img.data

scanner.stopImageServer();
```

## Using the Simulator

For development without hardware, use the Python simulator:

```bash
python scripts/simulator.py
# or: pixi run simulator
```

Then point examples at `127.0.0.1:9004`:

```bash
./build/examples/ReadCode --ip 127.0.0.1 --port 9004
```
