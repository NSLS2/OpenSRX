/**
 * @file ImageReadback.cpp
 * @brief Example: capture and receive images from the scanner via FTP.
 *
 * Starts an embedded FTP server, configures the scanner to send images to
 * it, triggers a read, and saves the received BMP image to disk.
 *
 * Usage:
 *   ImageReadback --ip 192.168.100.100 --port 9004 --local-ip 192.168.100.50
 *   ImageReadback --serial /dev/ttyUSB0 --local-ip 192.168.100.50
 */

#include <spdlog/spdlog.h>

#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ImageReadback");

    program.add_argument("--ip")
        .help("IP address of the scanner")
        .default_value(std::string("192.168.100.100"));
    program.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port")
        .help("Port number for socket connection")
        .default_value(9004)
        .scan<'i', int>();

    program.add_argument("--local-ip")
        .help("This machine's IP address as reachable from the scanner")
        .required();

    program.add_argument("--ftp-port")
        .help("Local FTP server port (default: 21)")
        .default_value(21)
        .scan<'i', int>();

    program.add_argument("-o", "--output")
        .help("Output BMP file path")
        .default_value(std::string("captured_image.bmp"));

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

    // Configure the scanner to save read-OK images as BMP via FTP
    scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);
    scanner.setParam<OpenSRX::OperationParam::IMAGE_SAVING_MODE>(OpenSRX::ImageSavingMode::LATEST_BANK_IMAGE);

    // Start the embedded FTP server and configure the scanner
    std::string localIP = program.get("--local-ip");
    uint16_t ftpPort = static_cast<uint16_t>(program.get<int>("--ftp-port"));
    scanner.startImageServer(localIP, {}, ftpPort);
    std::cout << "FTP server started on " << localIP << ":" << scanner.getImageServer()->getPort()
              << std::endl;

    // Trigger a read — the scanner will capture an image and FTP it to us
    std::cout << "Starting read (waiting for code + image)..." << std::endl;
    try {
        OpenSRX::Code code = scanner.startReading();
        std::cout << "Read result: " << code.data << std::endl;
    } catch (const std::runtime_error& e) {
        std::cerr << "Read failed: " << e.what() << std::endl;
        scanner.stopImageServer();
        return 1;
    }

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
    int bitsPerPixel = img.channels * 8;
    int paletteSize = (img.channels == 1) ? 256 * 4 : 0;
    int pixelOffset = 54 + paletteSize;
    int fileSize = pixelOffset + imageSize;

    // BMP file header (14 bytes)
    auto writeLE16 = [&](uint16_t v) { out.write(reinterpret_cast<const char*>(&v), 2); };
    auto writeLE32 = [&](uint32_t v) { out.write(reinterpret_cast<const char*>(&v), 4); };

    out.write("BM", 2);
    writeLE32(fileSize);
    writeLE32(0);   // reserved
    writeLE32(pixelOffset);  // pixel data offset

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
    writeLE32((img.channels == 1) ? 256 : 0);  // colors in palette
    writeLE32(0);     // important colors

    // Write grayscale palette for 8-bit images
    if (img.channels == 1) {
        for (int i = 0; i < 256; ++i) {
            uint8_t entry[4] = {static_cast<uint8_t>(i), static_cast<uint8_t>(i),
                                static_cast<uint8_t>(i), 0};
            out.write(reinterpret_cast<const char*>(entry), 4);
        }
    }

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
