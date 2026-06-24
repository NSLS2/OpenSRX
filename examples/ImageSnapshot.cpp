/**
 * @file ImageSnapshot.cpp
 * @brief Example: capture a snapshot image from the scanner.
 *
 * Sends the SHOT command to take a picture, then downloads the image
 * from the scanner's built-in FTP server and saves it as a BMP file.
 *
 * Usage:
 *   ImageSnapshot --ip 192.168.100.100 --port 9004
 *   ImageSnapshot --serial /dev/ttyUSB0
 */

#include <spdlog/spdlog.h>

#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

/// Write a decoded Image as a BMP file.
static void writeBMP(const std::string& path, const OpenSRX::Image& img) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("Cannot open output file: " + path);

    int rowBytes = img.width * img.channels;
    int rowPadding = (4 - (rowBytes % 4)) % 4;
    int imageSize = (rowBytes + rowPadding) * img.height;
    int bitsPerPixel = img.channels * 8;
    int paletteSize = (img.channels == 1) ? 256 * 4 : 0;
    int pixelOffset = 54 + paletteSize;
    int fileSize = pixelOffset + imageSize;

    auto writeLE16 = [&](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };
    auto writeLE32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };

    // File header
    out.write("BM", 2);
    writeLE32(fileSize);
    writeLE32(0);
    writeLE32(pixelOffset);

    // DIB header (BITMAPINFOHEADER)
    writeLE32(40);
    writeLE32(img.width);
    writeLE32(img.height);
    writeLE16(1);
    writeLE16(bitsPerPixel);
    writeLE32(0);
    writeLE32(imageSize);
    writeLE32(2835);
    writeLE32(2835);
    writeLE32((img.channels == 1) ? 256 : 0);
    writeLE32(0);

    // Grayscale palette for 8-bit images
    if (img.channels == 1) {
        for (int i = 0; i < 256; ++i) {
            uint8_t entry[4] = {static_cast<uint8_t>(i), static_cast<uint8_t>(i),
                                static_cast<uint8_t>(i), 0};
            out.write(reinterpret_cast<const char*>(entry), 4);
        }
    }

    // Pixel data (bottom-to-top)
    std::vector<uint8_t> padding(rowPadding, 0);
    for (int y = img.height - 1; y >= 0; --y) {
        out.write(reinterpret_cast<const char*>(img.data.data() + y * rowBytes), rowBytes);
        if (rowPadding > 0) out.write(reinterpret_cast<const char*>(padding.data()), rowPadding);
    }
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ImageSnapshot");

    program.add_argument("--ip")
        .help("IP address of the scanner")
        .default_value(std::string("192.168.100.100"));
    program.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port")
        .help("Port number for socket connection")
        .default_value(9004)
        .scan<'i', int>();

    program.add_argument("--bank")
        .help("Bank number for the snapshot (default: 1)")
        .default_value(1)
        .scan<'i', int>();

    program.add_argument("-o", "--output")
        .help("Output BMP file path")
        .default_value(std::string("snapshot.bmp"));

    program.add_argument("-d", "--debug")
        .help("Enable debug logging")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    spdlog::set_level(program.get<bool>("--debug") ? spdlog::level::debug : spdlog::level::info);

    // Connect to the scanner
    std::unique_ptr<OpenSRX::ICommInterface> iface;
    if (program.is_used("--serial")) {
        iface = std::make_unique<OpenSRX::SerialInterface>(program.get("--serial"));
    } else {
        iface = std::make_unique<OpenSRX::SocketInterface>(program.get("--ip"),
                                                           program.get<int>("--port"));
    }

    OpenSRX::Scanner scanner(*iface);
    std::cout << "Connected to " << scanner.getModel() << std::endl;

    // Capture snapshot and download via FTP
    int bank = program.get<int>("--bank");
    std::cout << "Capturing snapshot (bank " << bank << ")..." << std::endl;
    OpenSRX::Image img = scanner.captureSnapshot(bank);
    std::cout << "Received image: " << img.width << "x" << img.height << " (" << img.channels
              << " channels, " << img.data.size() << " bytes)" << std::endl;

    // Save to disk
    std::string outPath = program.get("-o");
    writeBMP(outPath, img);
    std::cout << "Saved BMP image to " << outPath << std::endl;

    return 0;
}
