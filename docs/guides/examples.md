# Examples {#examples}

The repository ships with example programs under the `examples/` directory.
All examples use [argparse](https://github.com/p-ranav/argparse) for CLI
argument handling and accept `--ip`/`--port` (TCP) or `--serial` (RS-232)
to connect to the scanner.

## VersionInfo

Prints the scanner model, firmware version, and library version:

```bash
./build/examples/VersionInfo --ip 192.168.100.100 --port 9004
```

## ReadCode

Triggers a barcode read and prints the decoded data:

```bash
./build/examples/ReadCode --ip 192.168.100.100 --port 9004
./build/examples/ReadCode --ip 192.168.100.100 --port 9004 --bank 2
```

## ReadCodeDetailed

Enables code vertex, center, and type appending, then triggers a read and
prints all metadata fields parsed into the `Code` struct:

```bash
./build/examples/ReadCodeDetailed --ip 192.168.100.100 --port 9004
```

## ImageReadback

Starts an embedded FTP server, triggers a read, receives the image via FTP,
and saves it as a BMP file:

```bash
./build/examples/ImageReadback --ip 192.168.100.100 --port 9004 \
    --local-ip 192.168.100.50 -o captured.bmp
```

Requires `--local-ip` (this machine's IP as reachable from the scanner).

## ImageSnapshot

Takes a camera snapshot (SHOT command, no barcode decode) and saves it as BMP:

```bash
./build/examples/ImageSnapshot --ip 192.168.100.100 --port 9004 \
    --local-ip 192.168.100.50 --bank 1 -o snapshot.bmp
```

## AdjustParams

Demonstrates reading and modifying scanner parameters (exposure, gain,
image format, FTP settings, etc.):

```bash
./build/examples/AdjustParams --ip 192.168.100.100 --port 9004 --bank 1
```

## Simulator

A Python-based scanner simulator is provided for development and testing
without physical hardware:

```bash
python scripts/simulator.py
# or: pixi run simulator
```

This opens a TCP server on port 9004 that emulates the SR-X command protocol,
including simulated barcode reads with generated camera images pushed via FTP.
The simulator supports all parameter read/write commands, image saving
configuration, and data appending options.

To use examples with the simulator:

```bash
./build/examples/ReadCode --ip 127.0.0.1 --port 9004
./build/examples/ImageReadback --ip 127.0.0.1 --port 9004 --local-ip 127.0.0.1
```
