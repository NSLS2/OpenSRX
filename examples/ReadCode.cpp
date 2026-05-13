/**
 * @file ReadCode.cpp
 * @brief Example: read a barcode from the scanner.
 *
 * Connects to the scanner, triggers a read, and prints the result.
 * The read blocks until a code is detected or the scanner times out.
 *
 * Usage:
 *   ReadCode --ip 192.168.100.100 --port 9004
 *   ReadCode --serial /dev/ttyUSB0
 */

#include <argparse/argparse.hpp>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("ReadCode");

    auto& group = program.add_mutually_exclusive_group(true);
    group.add_argument("--ip").help("IP address of the scanner");
    group.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port")
        .help("Port number (required with --ip)")
        .scan<'i', int>();

    program.add_argument("--bank")
        .help("Bank number to use for reading (1–16, omit for default)")
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
        iface = std::make_unique<OpenSRX::SocketInterface>(
            program.get("--ip"), program.get<int>("--port"));
    }

    OpenSRX::Scanner scanner(*iface);
    std::cout << "Connected to " << scanner.getModel() << " (FW " << scanner.getFirmwareVersion()
              << ")" << std::endl;

    // Trigger a read
    std::cout << "Starting read (waiting for code)..." << std::endl;
    std::string result;
    if (program.is_used("--bank")) {
        result = scanner.startReading(program.get<int>("--bank"));
    } else {
        result = scanner.startReading();
    }

    if (result == "ERROR") {
        std::cerr << "Read timed out or failed." << std::endl;
        return 1;
    }

    std::cout << "Read result: " << result << std::endl;
    return 0;
}
