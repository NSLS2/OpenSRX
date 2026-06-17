/**
 * @file ReadCodeDetailed.cpp
 * @brief Example: read a barcode with additional metadata (vertices, center, code type).
 *
 * Configures the scanner to append code vertex coordinates, center position,
 * and code type to the read result, then triggers a read and parses the
 * returned fields.
 *
 * The scanner appends fields to the barcode data separated by the
 * inter-delimiter character (default comma). Fields appear in the order
 * they are enabled: code vertex, code center, code type, bank number, angle.
 *
 * Usage:
 *   ReadCodeDetailed --ip 192.168.100.100 --port 9004
 *   ReadCodeDetailed --serial /dev/ttyUSB0
 */

#include <spdlog/spdlog.h>

#include <argparse/argparse.hpp>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ReadCodeDetailed");

    auto& group = program.add_mutually_exclusive_group(true);
    group.add_argument("--ip").help("IP address of the scanner");
    group.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port").help("Port number (required with --ip)").scan<'i', int>();

    program.add_argument("--bank")
        .help("Bank number to use for reading (1-16, omit for default)")
        .scan<'i', int>();

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
    std::cout << "Connected to " << scanner.getModel() << " (FW " << scanner.getFirmwareVersion()
              << ")" << std::endl;

    // Enable data appending options
    scanner.setParam<OpenSRX::OperationParam::CODE_VERTEX_APPENDING>(OpenSRX::Toggle::ENABLE);
    scanner.setParam<OpenSRX::OperationParam::CODE_CENTER_APPENDING>(OpenSRX::Toggle::ENABLE);
    scanner.setParam<OpenSRX::OperationParam::CODE_TYPE_APPENDING>(OpenSRX::Toggle::ENABLE);

    std::cout << "Enabled: code vertex, code center, code type appending" << std::endl;

    // Trigger a read — the library parses appended fields into the Code struct
    std::cout << "Starting read..." << std::endl;
    try {
        OpenSRX::Code result;
        if (program.is_used("--bank")) {
            result = scanner.startReading(program.get<int>("--bank"));
        } else {
            result = scanner.startReading();
        }

        std::cout << "\n=== Read Result ===" << std::endl;
        std::cout << "Code data:  " << result.data << std::endl;

        if (result.boundingBox) {
            auto& bb = *result.boundingBox;
            std::cout << "Bounding box corners:" << std::endl;
            std::cout << "  Top-left:     (" << bb.topLeft.x << ", " << bb.topLeft.y << ")"
                      << std::endl;
            std::cout << "  Top-right:    (" << bb.topRight.x << ", " << bb.topRight.y << ")"
                      << std::endl;
            std::cout << "  Bottom-right: (" << bb.bottomRight.x << ", " << bb.bottomRight.y << ")"
                      << std::endl;
            std::cout << "  Bottom-left:  (" << bb.bottomLeft.x << ", " << bb.bottomLeft.y << ")"
                      << std::endl;
        }

        if (result.center) {
            std::cout << "Code center:    (" << result.center->x << ", " << result.center->y << ")"
                      << std::endl;
        }

        if (result.codeType) {
            std::cout << "Code type:      " << *result.codeType << std::endl;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Read failed: " << e.what() << std::endl;
    }

    // Restore defaults
    scanner.setParam<OpenSRX::OperationParam::CODE_VERTEX_APPENDING>(OpenSRX::Toggle::DISABLE);
    scanner.setParam<OpenSRX::OperationParam::CODE_CENTER_APPENDING>(OpenSRX::Toggle::DISABLE);
    scanner.setParam<OpenSRX::OperationParam::CODE_TYPE_APPENDING>(OpenSRX::Toggle::DISABLE);
    std::cout << "Restored default appending settings." << std::endl;

    return 0;
}
