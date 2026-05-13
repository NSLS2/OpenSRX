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

std::string Scanner::checkResponse(const std::string& response) {
    if (response.substr(0, 3) == "ER,") {
        auto lastComma = response.rfind(',');
        if (lastComma <= 2) {
            throw std::runtime_error("Malformed error response: " + response);
        }

        int errCode;
        try {
            errCode = std::stoi(response.substr(lastComma + 1));
        } catch (...) {
            throw std::runtime_error("Malformed error response: " + response);
        }

        std::string msg = "Scanner error (code " + std::to_string(errCode) + "): ";

        switch (static_cast<ErrCode>(errCode)) {
            case ErrCode::CMD_UNDEFINED:
                throw std::invalid_argument(msg + "Undefined command");
            case ErrCode::MISMATCHED_CMD_FMT:
                throw std::invalid_argument(msg + "Mismatched command format");
            case ErrCode::PARAM1_OUT_OF_RANGE:
                throw std::out_of_range(msg + "Parameter 1 out of range");
            case ErrCode::PARAM2_OUT_OF_RANGE:
                throw std::out_of_range(msg + "Parameter 2 out of range");
            case ErrCode::PARAM2_NOT_IN_HEX:
                throw std::invalid_argument(msg + "Parameter 2 not in hex");
            case ErrCode::PARAM2_IN_HEX_BUT_OUT_OF_RANGE:
                throw std::out_of_range(msg + "Parameter 2 hex value out of range");
            case ErrCode::TWO_OR_MORE_MARKS_IN_PRESET_DATA:
                throw std::invalid_argument(msg + "Preset data incorrect");
            case ErrCode::AREA_SPECIFICATION_DATA_INCORRECT:
                throw std::invalid_argument(msg + "Area specification data incorrect");
            case ErrCode::FILE_DOES_NOT_EXIST:
                throw std::runtime_error(msg + "File does not exist");
            case ErrCode::TMM_LON_MM_OUT_OF_RANGE:
                throw std::out_of_range(msg + "TMM-LON mm out of range");
            case ErrCode::TMM_KEYENCE_COMMUNICATION_CANNOT_BE_CHECKED:
                throw std::runtime_error(msg + "Communication cannot be checked");
            case ErrCode::COMMAND_NOT_EXECUTABLE_IN_CURRENT_STATUS:
                throw std::runtime_error(msg + "Command not executable in current status");
            case ErrCode::BUFFER_OVERFLOW:
                throw std::overflow_error(msg + "Buffer overflow");
            case ErrCode::PARAMETER_LOAD_OR_SAVE_ERROR:
                throw std::runtime_error(msg + "Parameter load/save error");
            case ErrCode::CONNECTED_TO_AUTOID_NETWORK_NAVIGATOR:
                throw std::runtime_error(msg + "Connected to AutoID Network Navigator");
            case ErrCode::DEVICE_FAULT:
                throw std::runtime_error(msg + "Device fault");
            default:
                throw std::runtime_error(msg + "Unknown error");
        }
    }

    if (response.substr(0, 3) == "OK,") {
        // Find the value after the second comma: "OK,XX,value"
        auto secondComma = response.find(',', 3);
        if (secondComma != std::string::npos) {
            return response.substr(secondComma + 1);
        }
        // Write responses have no value: "OK,WB"
        return "";
    }

    return response;
}

}  // namespace OpenSRX
