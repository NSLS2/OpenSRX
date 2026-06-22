#include "OpenSRX/Scanner.hpp"

#include <spdlog/spdlog.h>

#include <sstream>
#include <vector>

namespace OpenSRX {

std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw) {
    auto commaPos = raw.find(',');
    if (commaPos == std::string::npos)
        throw std::runtime_error("Unexpected version info format: " + raw);

    std::string model = raw.substr(0, commaPos);
    std::string firmwareVersion = raw.substr(commaPos + 1);
    return {model, firmwareVersion};
}

static std::string formatBank(int bank) {
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << bank;
    return ss.str();
}

Scanner::Scanner(ICommInterface& comm) : comm(comm) {
    spdlog::info("Connecting to scanner at {}", comm.describe());
    spdlog::info("Obtaining version information...");
    std::string raw = checkResponse(comm.sendCommand("KEYENCE"));
    auto [model, firmware] = parseVersionInfo(raw);
    this->model = model;
    this->firmwareVersion = firmware;
    spdlog::info("Connected to scanner, model: {}, firmware version: {}", model, firmware);
    this->macAddress = checkResponse(comm.sendCommand("EMAC"));
}

// ─── Reading and tuning ─────────────────────────────────────────────────────

Code Scanner::startReading() {
    std::string raw = comm.sendCommandUnlocked("LON");
    if (raw == "ERROR") throw std::runtime_error("Read failed or timed out");
    return parseReadResult(raw);
}

Code Scanner::startReading(int bank) {
    std::string raw = comm.sendCommandUnlocked("LON," + formatBank(bank));
    if (raw == "ERROR") throw std::runtime_error("Read failed or timed out");
    return parseReadResult(raw);
}

void Scanner::stopReading() { comm.sendCommandUnlocked("LOFF"); }

void Scanner::startQuickSetupCodeReading() { checkResponse(comm.sendCommand("RCON")); }

void Scanner::finishQuickSetupCodeReading() { checkResponse(comm.sendCommand("RCOFF")); }

std::string Scanner::checkQuickSetupCodeResult() {
    return checkResponse(comm.sendCommand("RCCHK"));
}

void Scanner::readingRateTest() { checkResponse(comm.sendCommandUnlocked("TEST1")); }

void Scanner::readingRateTest(int bank) {
    checkResponse(comm.sendCommandUnlocked("TEST1," + formatBank(bank)));
}

void Scanner::readTimeTest() { checkResponse(comm.sendCommandUnlocked("TEST2")); }

void Scanner::readTimeTest(int bank) {
    checkResponse(comm.sendCommandUnlocked("TEST2," + formatBank(bank)));
}

void Scanner::quitTestMode() { checkResponse(comm.sendCommandUnlocked("QUIT")); }

// ─── I/O terminal control ───────────────────────────────────────────────────

bool Scanner::getInputTerminalState(int terminalNumber) {
    std::string val = checkResponse(comm.sendCommand("INCHK," + std::to_string(terminalNumber)));
    return val == "ON";
}

void Scanner::turnOnOutputTerminal(int terminalNumber) {
    checkResponse(comm.sendCommand("OUTON," + std::to_string(terminalNumber)));
}

void Scanner::turnOffOutputTerminal(int terminalNumber) {
    checkResponse(comm.sendCommand("OUTOFF," + std::to_string(terminalNumber)));
}

void Scanner::turnOnAllOutputTerminals() { checkResponse(comm.sendCommand("ALLON")); }

void Scanner::turnOffAllOutputTerminals() { checkResponse(comm.sendCommand("ALLOFF")); }

// ─── Reset and buffer ────────────────────────────────────────────────────────

void Scanner::reset() { checkResponse(comm.sendCommand("RESET")); }

void Scanner::clearSendBuffer() { checkResponse(comm.sendCommand("BCLR")); }

// ─── Image and focus ─────────────────────────────────────────────────────────

std::string Scanner::captureImage(int bank) {
    return checkResponse(comm.sendCommand("SHOT," + formatBank(bank)));
}

void Scanner::adjustFocus() { checkResponse(comm.sendCommandUnlocked("FTUNE")); }

std::tuple<bool, TuningAdvice, TuningFailureReason> Scanner::startTuning(int bank) {
    std::string result = comm.sendCommandUnlocked("TUNE," + formatBank(bank));
    // Response format:
    //   Success: "Tuning SUCCEEDED,<time>ms,00000<advice>00"
    //   Failure: "Tuning FAILED,<time>ms,00000<advice>0<reason>"
    bool succeeded = result.find("SUCCEEDED") != std::string::npos;

    auto lastComma = result.rfind(',');
    if (lastComma == std::string::npos)
        throw std::runtime_error("Unexpected tuning response format: " + result);

    std::string statusField = result.substr(lastComma + 1);
    if (statusField.size() < 8)
        throw std::runtime_error("Unexpected tuning status field: " + statusField);

    int advice = statusField[5] - '0';
    int reason = statusField[7] - '0';

    return {succeeded, static_cast<TuningAdvice>(advice), static_cast<TuningFailureReason>(reason)};
}

void Scanner::stopTuning() { checkResponse(comm.sendCommandUnlocked("TQUIT")); }

// ─── Pointer control ─────────────────────────────────────────────────────────

void Scanner::enablePointer() { checkResponse(comm.sendCommand("AMON")); }

void Scanner::disablePointer() { checkResponse(comm.sendCommand("AMOFF")); }

// ─── Time settings ───────────────────────────────────────────────────────────

void Scanner::setTime(const Timestamp& timestamp) {
    std::ostringstream cmd;
    cmd << "TMSET," << std::setw(4) << std::setfill('0') << timestamp.year << std::setw(2)
        << std::setfill('0') << timestamp.month << std::setw(2) << std::setfill('0')
        << timestamp.day << std::setw(2) << std::setfill('0') << timestamp.hour << std::setw(2)
        << std::setfill('0') << timestamp.minute << std::setw(2) << std::setfill('0')
        << timestamp.second;
    checkResponse(comm.sendCommand(cmd.str()));
}

Timestamp Scanner::getTime() {
    std::string t = checkResponse(comm.sendCommand("TMGET"));
    // Format: YYYYMMDDhhmmss (14 characters)
    int year = std::stoi(t.substr(0, 4));
    int month = std::stoi(t.substr(4, 2));
    int day = std::stoi(t.substr(6, 2));
    int hour = std::stoi(t.substr(8, 2));
    int minute = std::stoi(t.substr(10, 2));
    int second = std::stoi(t.substr(12, 2));
    return Timestamp(second, minute, hour, day, month, year);
}

// ─── Status queries ──────────────────────────────────────────────────────────

CommandStatus Scanner::getCommandStatus() {
    std::string val = checkResponse(comm.sendCommand("CMDSTAT"));
    if (val == "none") return CommandStatus::NO_PROCESSING;
    if (val == "wait") return CommandStatus::WAIT_FOR_SETTING;
    if (val == "update") return CommandStatus::UPDATING;
    throw std::runtime_error("Unknown command status: " + val);
}

ErrorStatus Scanner::getErrorStatus() {
    std::string val = checkResponse(comm.sendCommand("ERRSTAT"));
    if (val == "none") return ErrorStatus::NO_ERROR;
    if (val == "system") return ErrorStatus::SYSTEM_ERROR;
    if (val == "update") return ErrorStatus::UPDATE_ERROR;
    if (val == "cfg") return ErrorStatus::SET_VALUE_ERROR;
    if (val == "ip") return ErrorStatus::DUPLICATE_IP_ERROR;
    if (val == "over") return ErrorStatus::BUFF_OVERFLOW_ERROR;
    if (val == "plc") return ErrorStatus::PLC_LINK_ERROR;
    if (val == "profinet") return ErrorStatus::PROFINET_ERROR;
    if (val == "lua") return ErrorStatus::LUA_SCRIPT_ERROR;
    if (val == "hostconnect") return ErrorStatus::CONNECTION_ERROR;
    throw std::runtime_error("Unknown error status: " + val);
}

BusyStatus Scanner::getBusyStatus() {
    std::string val = checkResponse(comm.sendCommand("BUSYSTAT"));
    if (val == "none") return BusyStatus::IDLE;
    if (val == "trg") return BusyStatus::TRG_BUSY;
    if (val == "update") return BusyStatus::UPDATE_PROCESSING;
    if (val == "file") return BusyStatus::SAVING_FILE;
    if (val == "af") return BusyStatus::AUTO_FOCUSING;
    throw std::runtime_error("Unknown busy status: " + val);
}

// ─── Settings management ─────────────────────────────────────────────────────

void Scanner::copyBankConfiguration(int sourceBank, int targetBank) {
    checkResponse(
        comm.sendCommand("BCOPY," + formatBank(sourceBank) + "," + formatBank(targetBank)));
}

void Scanner::saveSettings() { checkResponse(comm.sendCommand("SAVE")); }

void Scanner::loadSavedSettings() { checkResponse(comm.sendCommand("LOAD")); }

void Scanner::resetToFactorySettings() { checkResponse(comm.sendCommand("DFLT")); }

void Scanner::saveBackupSettings(int backupNumber) {
    checkResponse(comm.sendCommand("BSAVE," + std::to_string(backupNumber)));
}

void Scanner::loadBackupSettings(int backupNumber) {
    checkResponse(comm.sendCommand("BLOAD," + std::to_string(backupNumber)));
}

// ─── Error clearing ──────────────────────────────────────────────────────────

void Scanner::clearFTPCommsError() { checkResponse(comm.sendCommand("HCLR")); }

void Scanner::clearPLCLinkError() { checkResponse(comm.sendCommand("PCLR")); }

// ─── Image server ────────────────────────────────────────────────────────────

void Scanner::startImageServer(const std::string& localIP, ImageSaveConfig saveConfig,
                               uint16_t port, const std::string& username,
                               const std::string& password) {
    if (imageServer && imageServer->isRunning())
        throw std::runtime_error("Image server is already running");

    imageServer = std::make_unique<ImageServer>(port, username, password);
    imageServer->start();

    // Configure the scanner's FTP parameters to point at our server
    setParam<CommParam::FTP_REMOTE_IP>(localIP);
    setParam<CommParam::FTP_USER_NAME>(username);
    setParam<CommParam::FTP_PASSWORD>(password);
    setParam<CommParam::FTP_REMOTE_PORT>(static_cast<int>(imageServer->getPort()));
    setParam<CommParam::FTP_PASSIVE_MODE>(Toggle::ENABLE);

    // Save current image saving destinations so we can restore on stop
    savedImageDests = std::make_unique<SavedImageDests>();
    savedImageDests->readOK = getParam<OperationParam::SAVE_DEST_READ_OK>();
    savedImageDests->verificationNG = getParam<OperationParam::SAVE_DEST_VERIFICATION_NG>();
    savedImageDests->readError = getParam<OperationParam::SAVE_DEST_READ_ERROR>();
    savedImageDests->unstable = getParam<OperationParam::SAVE_DEST_UNSTABLE>();
    savedImageDests->capture = getParam<OperationParam::SAVE_DEST_CAPTURE>();

    // Set selected image types to SEND_BY_FTP
    if (saveConfig.readOK)
        setParam<OperationParam::SAVE_DEST_READ_OK>(ImageSavingDestination::SEND_BY_FTP);
    if (saveConfig.verificationNG)
        setParam<OperationParam::SAVE_DEST_VERIFICATION_NG>(ImageSavingDestination::SEND_BY_FTP);
    if (saveConfig.readError)
        setParam<OperationParam::SAVE_DEST_READ_ERROR>(ImageSavingDestination::SEND_BY_FTP);
    if (saveConfig.unstable)
        setParam<OperationParam::SAVE_DEST_UNSTABLE>(ImageSavingDestination::SEND_BY_FTP);
    if (saveConfig.capture)
        setParam<OperationParam::SAVE_DEST_CAPTURE>(ImageSavingDestination::SEND_BY_FTP);

    // Communication settings (FTP IP, port, credentials) require a SAVE
    // command before they take effect on the scanner.
    saveSettings();
}

void Scanner::stopImageServer() {
    if (imageServer) {
        imageServer->stop();
        imageServer.reset();
    }

    // Restore previous image saving destinations
    if (savedImageDests) {
        setParam<OperationParam::SAVE_DEST_READ_OK>(savedImageDests->readOK);
        setParam<OperationParam::SAVE_DEST_VERIFICATION_NG>(savedImageDests->verificationNG);
        setParam<OperationParam::SAVE_DEST_READ_ERROR>(savedImageDests->readError);
        setParam<OperationParam::SAVE_DEST_UNSTABLE>(savedImageDests->unstable);
        setParam<OperationParam::SAVE_DEST_CAPTURE>(savedImageDests->capture);
        savedImageDests.reset();
        saveSettings();
    }
}

Image Scanner::waitForImage() {
    if (!imageServer || !imageServer->isRunning())
        throw std::runtime_error("Image server is not running");
    return imageServer->waitForImage();
}

bool Scanner::tryGetImage(Image& image) {
    if (!imageServer || !imageServer->isRunning()) return false;
    return imageServer->tryGetImage(image);
}

std::deque<Image> Scanner::getImages() {
    if (!imageServer || !imageServer->isRunning()) return {};
    return imageServer->getImages();
}

void Scanner::setImageCallback(std::function<void(const Image&)> cb) {
    if (!imageServer) throw std::runtime_error("Image server has not been created yet");
    imageServer->setImageCallback(std::move(cb));
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

Code Scanner::parseReadResult(const std::string& raw) {
    Code code;

    // Read the inter-delimiter character (hex byte, default 0x2C = comma)
    std::string delimHex = getParam<OperationParam::INTER_DELIMITER>();
    char delim = ',';
    try {
        delim = static_cast<char>(std::stoi(delimHex, nullptr, 16));
    } catch (...) {
        // fall back to comma
    }

    // Split the raw result by delimiter
    std::vector<std::string> fields;
    std::istringstream stream(raw);
    std::string token;
    while (std::getline(stream, token, delim)) {
        fields.push_back(token);
    }

    if (fields.empty()) {
        code.data = raw;
        return code;
    }

    // First field is always the barcode data
    code.data = fields[0];
    size_t idx = 1;

    // Parse appended fields in the order the scanner appends them:
    // CODE_VERTEX_APPENDING (308): 8 ints (TL.x, TL.y, TR.x, TR.y, BR.x, BR.y, BL.x, BL.y)
    if (getParam<OperationParam::CODE_VERTEX_APPENDING>() == Toggle::ENABLE) {
        if (idx + 8 <= fields.size()) {
            BoundingBox bb;
            bb.topLeft = {std::stoi(fields[idx]), std::stoi(fields[idx + 1])};
            bb.topRight = {std::stoi(fields[idx + 2]), std::stoi(fields[idx + 3])};
            bb.bottomRight = {std::stoi(fields[idx + 4]), std::stoi(fields[idx + 5])};
            bb.bottomLeft = {std::stoi(fields[idx + 6]), std::stoi(fields[idx + 7])};
            code.boundingBox = bb;
            idx += 8;
        }
    }

    // CODE_CENTER_APPENDING (309): 2 ints (cx, cy)
    if (getParam<OperationParam::CODE_CENTER_APPENDING>() == Toggle::ENABLE) {
        if (idx + 2 <= fields.size()) {
            code.center = Point{std::stoi(fields[idx]), std::stoi(fields[idx + 1])};
            idx += 2;
        }
    }

    // CODE_TYPE_APPENDING (301): 1 string
    if (getParam<OperationParam::CODE_TYPE_APPENDING>() == Toggle::ENABLE) {
        if (idx < fields.size()) {
            code.codeType = fields[idx];
            idx += 1;
        }
    }

    // BANK_NUMBER_APPENDING (303): 1 int
    if (getParam<OperationParam::BANK_NUMBER_APPENDING>() == Toggle::ENABLE) {
        if (idx < fields.size()) {
            code.bankNumber = std::stoi(fields[idx]);
            idx += 1;
        }
    }

    // ANGLE_APPENDING (371): 1 float
    if (getParam<OperationParam::ANGLE_APPENDING>() == Toggle::ENABLE) {
        if (idx < fields.size()) {
            code.angle = std::stod(fields[idx]);
            idx += 1;
        }
    }

    return code;
}

}  // namespace OpenSRX
