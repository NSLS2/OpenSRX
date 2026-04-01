#include <argparse/argparse.hpp>
#include <iostream>
#include <memory>

#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/Scanner.hpp"
#include "OpenSRX/SerialInterface.hpp"
#include "OpenSRX/SocketInterface.hpp"

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("VersionInfo");

    auto& group = program.add_mutually_exclusive_group(true);

    group.add_argument("--ip").help("IP address of the scanner");
    group.add_argument("--serial").help("Serial port device path (e.g. /dev/ttyUSB0)");

    program.add_argument("--port")
        .help("Port number of the scanner (required with --ip)")
        .scan<'i', int>();

    program.add_argument("-d", "--debug").help("Enable debug logging").default_value(false).implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    if (program.is_used("--ip") && !program.is_used("--port")) {
        std::cerr << "Error: --port is required when using --ip" << std::endl;
        std::cerr << program;
        return 1;
    }

    // OpenSRX uses spdlog for logging. Set the log level based on whether the user passed the --debug flag.
    if (program.get<bool>("--debug")) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    std::cout << "OpenSRX " << OpenSRX::GetVersion<std::string>() << std::endl;

    // Can also get the library version as a tuple of integers (major, minor, patch) if needed
    // auto versionTuple = OpenSRX::GetVersion<OpenSRX::VersionTuple>();
    // std::cout << "Version tuple: (" << std::get<0>(versionTuple) << ", "
    //           << std::get<1>(versionTuple) << ", " << std::get<2>(versionTuple) << ")" <<
    //           std::endl;

    std::unique_ptr<OpenSRX::ICommInterface> iface;
    if (program.is_used("--serial")) {
        iface = std::make_unique<OpenSRX::SerialInterface>(program.get("--serial"));
    } else {
        auto ip = program.get("--ip");
        auto port = program.get<int>("--port");
        iface = std::make_unique<OpenSRX::SocketInterface>(ip, port);
    }

    OpenSRX::Scanner scanner(*iface);
    std::cout << "Model: " << scanner.getModel() << std::endl;
    std::cout << "Firmware Version: " << scanner.getFirmwareVersion() << std::endl;
    return 0;
}
