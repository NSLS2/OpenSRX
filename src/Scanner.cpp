#include "OpenSRX/Scanner.hpp"

namespace OpenSRX {

std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw) {
    // The response to the "KEYENCE" command is expected to be in the format "Model,FirmwareVersion"
    auto commaPos = raw.find(',');
    if (commaPos == std::string::npos)
        throw std::runtime_error("Unexpected version info format: " + raw);

    std::string model = raw.substr(0, commaPos);
    std::string firmwareVersion = raw.substr(commaPos + 1);
    return {model, firmwareVersion};
}

Scanner::Scanner(ICommInterface& comm) : comm(comm) {
    spdlog::info("Connecting to scanner at {}", comm.describe());
    spdlog::info("Obtaining version information...");
    std::string raw = comm.sendCommand("KEYENCE");
    auto [model, firmware] = parseVersionInfo(raw);
    this->model = model;
    this->firmwareVersion = firmware;
    spdlog::info("Connected to scanner, model: {}, firmware version: {}", model, firmware);
}

}  // namespace OpenSRX
