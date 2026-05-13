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

#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ImageReadback");

    auto& group = program.add_mutually_exclusive_group(true);
    group.add_argument("--ip").help("IP address of the scanner");
    group.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port").help("Port number (required with --ip)").scan<'i', int>();

    program.add_argument("--local-ip")
        .help("This machine's IP address as reachable from the scanner")
        .required();

    program.add_argument("-o", "--output")
        .help("Output file path for the raw pixel data")
        .default_value(std::string("captured_image.raw"));

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

    // Configure the scanner to save read-OK images as BMP via FTP
    scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);

    // Start the embedded FTP server and configure the scanner
    std::string localIP = program.get("--local-ip");
    scanner.startImageServer(localIP);
    std::cout << "FTP server started on " << localIP << ":" << scanner.getImageServer()->getPort()
              << std::endl;

    // Trigger a read — the scanner will capture an image and FTP it to us
    std::cout << "Starting read (waiting for code + image)..." << std::endl;
    std::string code = scanner.startReading();
    if (code == "ERROR") {
        std::cerr << "Read failed or timed out." << std::endl;
        scanner.stopImageServer();
        return 1;
    }
    std::cout << "Read result: " << code << std::endl;

    // Wait for the image to arrive via FTP
    std::cout << "Waiting for image..." << std::endl;
    OpenSRX::Image img = scanner.waitForImage();
    std::cout << "Received image: " << img.width << "x" << img.height << " (" << img.channels
              << " channels, " << img.data.size() << " bytes)" << std::endl;

    // Write raw pixel data to file
    std::string outPath = program.get("-o");
    std::ofstream out(outPath, std::ios::binary);
    out.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
    std::cout << "Saved raw image data to " << outPath << std::endl;

    scanner.stopImageServer();
    return 0;
}
