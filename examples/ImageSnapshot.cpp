/**
 * @file ImageSnapshot.cpp
 * @brief Example: take an image snapshot from the scanner via FTP.
 *
 * Starts an embedded FTP server, configures the scanner to send capture
 * images to it, triggers a snapshot (SHOT command), and saves the received
 * BMP image to disk. Unlike ImageReadback, this does not perform a barcode
 * read — it simply captures the current camera view.
 *
 * Usage:
 *   ImageSnapshot --ip 192.168.100.100 --port 9004 --local-ip 192.168.100.50
 *   ImageSnapshot --serial /dev/ttyUSB0 --local-ip 192.168.100.50
 */

#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ImageSnapshot");

    auto& group = program.add_mutually_exclusive_group(true);
    group.add_argument("--ip").help("IP address of the scanner");
    group.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port").help("Port number (required with --ip)").scan<'i', int>();

    program.add_argument("--local-ip")
        .help("This machine's IP address as reachable from the scanner")
        .required();

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

    if (program.is_used("--ip") && !program.is_used("--port")) {
        std::cerr << "Error: --port is required when using --ip" << std::endl;
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

    // Configure the scanner to save capture images as BMP via FTP
    scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);

    // Start the FTP server — enable only capture image saving
    OpenSRX::ImageSaveConfig saveConfig;
    saveConfig.readOK = false;
    saveConfig.capture = true;

    std::string localIP = program.get("--local-ip");
    scanner.startImageServer(localIP, saveConfig);
    std::cout << "FTP server started on " << localIP << ":" << scanner.getImageServer()->getPort()
              << std::endl;

    // Take a snapshot
    int bank = program.get<int>("--bank");
    std::cout << "Taking snapshot (bank " << bank << ")..." << std::endl;
    scanner.captureImage(bank);

    // Wait for the image to arrive via FTP
    std::cout << "Waiting for image..." << std::endl;
    OpenSRX::Image img = scanner.waitForImage();
    std::cout << "Received image: " << img.width << "x" << img.height << " (" << img.channels
              << " channels, " << img.data.size() << " bytes)" << std::endl;

    // Write image as BMP file
    std::string outPath = program.get("-o");
    std::ofstream out(outPath, std::ios::binary);

    int rowBytes = img.width * img.channels;
    int rowPadding = (4 - (rowBytes % 4)) % 4;
    int imageSize = (rowBytes + rowPadding) * img.height;
    int fileSize = 54 + imageSize;
    int bitsPerPixel = img.channels * 8;

    // BMP file header (14 bytes)
    auto writeLE16 = [&](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };
    auto writeLE32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };

    out.write("BM", 2);
    writeLE32(fileSize);
    writeLE32(0);   // reserved
    writeLE32(54);  // pixel data offset

    // DIB header (BITMAPINFOHEADER, 40 bytes)
    writeLE32(40);
    writeLE32(img.width);
    writeLE32(img.height);
    writeLE16(1);  // color planes
    writeLE16(bitsPerPixel);
    writeLE32(0);  // no compression
    writeLE32(imageSize);
    writeLE32(2835);  // horizontal resolution (72 DPI)
    writeLE32(2835);  // vertical resolution (72 DPI)
    writeLE32(0);     // colors in palette
    writeLE32(0);     // important colors

    // Pixel data — BMP stores rows bottom-to-top
    std::vector<uint8_t> padding(rowPadding, 0);
    for (int y = img.height - 1; y >= 0; --y) {
        out.write(reinterpret_cast<const char*>(img.data.data() + y * rowBytes), rowBytes);
        if (rowPadding > 0) out.write(reinterpret_cast<const char*>(padding.data()), rowPadding);
    }

    std::cout << "Saved BMP image to " << outPath << std::endl;

    scanner.stopImageServer();
    return 0;
}
