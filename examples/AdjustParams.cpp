/**
 * @file AdjustParams.cpp
 * @brief Example: read and modify scanner parameters.
 *
 * Demonstrates reading and writing bank, operation, tuning, and
 * communication parameters using the convenience aliases
 * (getParam / setParam).
 *
 * Usage:
 *   AdjustParams --ip 192.168.100.100 --port 9004
 *   AdjustParams --serial /dev/ttyUSB0
 */

#include <spdlog/spdlog.h>

#include <argparse/argparse.hpp>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("AdjustParams");

    program.add_argument("--ip")
        .help("IP address of the scanner")
        .default_value(std::string("192.168.100.100"));
    program.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port")
        .help("Port number for socket connection")
        .default_value(9004)
        .scan<'i', int>();

    program.add_argument("--bank")
        .help("Bank number for bank parameter operations (default: 1)")
        .default_value(1)
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
    int bank = program.get<int>("--bank");
    std::cout << "Connected to " << scanner.getModel() << " (FW " << scanner.getFirmwareVersion()
              << ")" << std::endl;

    // ── Read bank parameters ────────────────────────────────────────────────
    std::cout << "\n=== Bank " << bank << " Parameters ===" << std::endl;

    int exposure = scanner.getParam<OpenSRX::BankParam::EXPOSURE_TIME>(bank);
    std::cout << "Exposure time: " << exposure << std::endl;

    int gain = scanner.getParam<OpenSRX::BankParam::GAIN>(bank);
    std::cout << "Gain: " << gain << std::endl;

    auto lighting = scanner.getParam<OpenSRX::BankParam::INTERNAL_LIGHTING_TYPE>(bank);
    std::cout << "Internal lighting type: " << static_cast<int>(lighting) << std::endl;

    // ── Read operation parameters ───────────────────────────────────────────
    std::cout << "\n=== Operation Parameters ===" << std::endl;

    auto imageFormat = scanner.getParam<OpenSRX::OperationParam::IMAGE_FORMAT>();
    std::cout << "Image format: " << (imageFormat == OpenSRX::ImageFormat::BMP ? "BMP" : "JPG")
              << std::endl;

    auto saveDest = scanner.getParam<OpenSRX::OperationParam::SAVE_DEST_READ_OK>();
    std::cout << "Save dest (read OK): " << static_cast<int>(saveDest) << std::endl;

    // ── Modify a parameter ──────────────────────────────────────────────────
    std::cout << "\n=== Adjusting Parameters ===" << std::endl;

    // Set image format to BMP
    scanner.setParam<OpenSRX::OperationParam::IMAGE_FORMAT>(OpenSRX::ImageFormat::BMP);
    std::cout << "Set image format to BMP" << std::endl;

    // Adjust exposure time
    std::cout << "Setting exposure time to 500..." << std::endl;
    scanner.setParam<OpenSRX::BankParam::EXPOSURE_TIME>(bank, 500);
    int newExposure = scanner.getParam<OpenSRX::BankParam::EXPOSURE_TIME>(bank);
    std::cout << "New exposure time: " << newExposure << std::endl;

    // Restore original exposure
    std::cout << "Restoring exposure time to " << exposure << "..." << std::endl;
    scanner.setParam<OpenSRX::BankParam::EXPOSURE_TIME>(bank, exposure);

    // ── Read communication parameters ───────────────────────────────────────
    std::cout << "\n=== Communication Parameters ===" << std::endl;

    std::string ftpIP = scanner.getParam<OpenSRX::CommParam::FTP_REMOTE_IP>();
    std::cout << "FTP remote IP: " << (ftpIP.empty() ? "(not set)" : ftpIP) << std::endl;

    // ── Save settings ───────────────────────────────────────────────────────
    // Uncomment the line below to persist changes to non-volatile memory:
    // scanner.saveSettings();

    std::cout << "\nDone." << std::endl;
    return 0;
}
