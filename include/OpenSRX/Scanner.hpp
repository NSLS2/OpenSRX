#pragma once

#include <iomanip>
#include <sstream>

#include "OpenSRX/ICommInterface.hpp"
#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/ParamTraits.hpp"
#include "OpenSRX/Timestamp.hpp"

namespace OpenSRX {

enum class CommandStatus {
    NO_PROCESSING = 0,
    WAIT_FOR_SETTING = 1,
    UPDATING = 2,
};

enum class ErrorStatus {
    NO_ERROR = 0,
    SYSTEM_ERROR = 1,
    UPDATE_ERROR = 2,
    SET_VALUE_ERROR = 3,
    DUPLICATE_IP_ERROR = 4,
    BUFF_OVERFLOW_ERROR = 5,
    PLC_LINK_ERROR = 6,
    PROFINET_ERROR = 7,
    LUA_SCRIPT_ERROR = 8,
    CONNECTION_ERROR = 9,
};

enum class BusyStatus {
    IDLE = 0,
    TRG_BUSY = 1,
    UPDATE_PROCESSING = 2,
    SAVING_FILE = 3,
    AUTO_FOCUSING = 4,
};

enum class TuningAdvice {
    NONE = 0,
    USE_AN_IMAGE_FILTER = 1,
    CONSIDER_INSTALLATION_LIGHTING_PRINTING_CONDITIONS = 2,
    BRIGHTNESS_INSUFFICIENT = 4,
};

enum class TuningFailureReason {
    CODE_DETECTION_IMPOSSIBLE = 1,
    UNSTABLE_READING = 2,
};

std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw);

class Scanner {
   public:
    Scanner(ICommInterface& comm);
    ~Scanner() = default;

    std::string startReading();
    std::string startReading(int bank);
    void stopReading();

    void startQuickSetupCodeReading();
    void finishQuickSetupCodeReading();
    std::string checkQuickSetupCodeResult();

    void readingRateTest();
    void readingRateTest(int bank);
    void readTimeTest();
    void readTimeTest(int bank);
    void quitTestMode();

    bool getInputTerminalState(int terminalNumber);
    void turnOnOutputTerminal(int terminalNumber);
    void turnOffOutputTerminal(int terminalNumber);
    void turnOnAllOutputTerminals();
    void turnOffAllOutputTerminals();

    void reset();

    void clearSendBuffer();

    std::string captureImage(int bank);

    void adjustFocus();
    std::tuple<bool, TuningAdvice, TuningFailureReason> startTuning(int bank);
    void stopTuning();

    std::string getModel() const { return model; }
    std::string getFirmwareVersion() const { return firmwareVersion; }
    std::string getMacAddress() const { return macAddress; }

    void enablePointer();
    void disablePointer();

    void setTime(const Timestamp& timestamp);
    Timestamp getTime();

    CommandStatus getCommandStatus();
    ErrorStatus getErrorStatus();
    BusyStatus getBusyStatus();

    void copyBankConfiguration(int sourceBank, int targetBank);

    void saveSettings();
    void loadSavedSettings();
    void resetToFactorySettings();

    void saveBackupSettings(int backupNumber);
    void loadBackupSettings(int backupNumber);

    void clearFTPCommsError();
    void clearPLCLinkError();

    // ─── Bank parameters (WB/RB) ───────────────────────────────────────────

    /**
     * @brief Read a bank parameter.
     * @tparam P The BankParam enumerator (determines the return type).
     * @param bank Bank number (1–16).
     * @return The parameter value in its natural C++ type.
     */
    template <BankParam P>
    auto getParam(int bank) -> ParamCppT<paramType(P)> {
        std::ostringstream cmd;
        cmd << "RB," << std::setw(2) << std::setfill('0') << bank << std::setw(3)
            << std::setfill('0') << static_cast<int>(P);
        std::string raw = comm.sendCommand(cmd.str());
        return parseParam<paramType(P)>(raw);
    }

    /**
     * @brief Write a bank parameter.
     * @tparam P The BankParam enumerator (determines the value type).
     * @param bank Bank number (1–16).
     * @param value The value to set.
     */
    template <BankParam P>
    void setParam(int bank, const ParamCppT<paramType(P)>& value) {
        std::ostringstream cmd;
        cmd << "WB," << std::setw(2) << std::setfill('0') << bank << std::setw(3)
            << std::setfill('0') << static_cast<int>(P) << "," << formatParam<paramType(P)>(value);
        comm.sendCommand(cmd.str());
    }

    // ─── Tuning parameters (WC/RC) ─────────────────────────────────────────

    /**
     * @brief Read a tuning parameter.
     * @tparam P The TuningParam enumerator.
     * @return The parameter value in its natural C++ type.
     */
    template <TuningParam P>
    auto getParam() -> ParamCppT<paramType(P)> {
        std::string cmd = "RC," + std::to_string(static_cast<int>(P));
        std::string raw = comm.sendCommand(cmd);
        return parseParam<paramType(P)>(raw);
    }

    /**
     * @brief Write a tuning parameter.
     * @tparam P The TuningParam enumerator.
     * @param value The value to set.
     */
    template <TuningParam P>
    void setParam(const ParamCppT<paramType(P)>& value) {
        std::string cmd =
            "WC," + std::to_string(static_cast<int>(P)) + "," + formatParam<paramType(P)>(value);
        comm.sendCommand(cmd);
    }

    // ─── Operation parameters (WP/RP) ──────────────────────────────────────

    /**
     * @brief Read an operation parameter.
     * @tparam P The OperationParam enumerator.
     * @return The parameter value in its natural C++ type.
     */
    template <OperationParam P>
    auto getParam() -> ParamCppT<paramType(P)> {
        std::string cmd = "RP," + std::to_string(static_cast<int>(P));
        std::string raw = comm.sendCommand(cmd);
        return parseParam<paramType(P)>(raw);
    }

    /**
     * @brief Write an operation parameter.
     * @tparam P The OperationParam enumerator.
     * @param value The value to set.
     */
    template <OperationParam P>
    void setParam(const ParamCppT<paramType(P)>& value) {
        std::string cmd =
            "WP," + std::to_string(static_cast<int>(P)) + "," + formatParam<paramType(P)>(value);
        comm.sendCommand(cmd);
    }

    // ─── Communication parameters (WN/RN) ──────────────────────────────────

    /**
     * @brief Read a communication parameter.
     * @tparam P The CommParam enumerator.
     * @return The parameter value in its natural C++ type.
     */
    template <CommParam P>
    auto getParam() -> ParamCppT<paramType(P)> {
        std::string cmd = "RN," + std::to_string(static_cast<int>(P));
        std::string raw = comm.sendCommand(cmd);
        return parseParam<paramType(P)>(raw);
    }

    /**
     * @brief Write a communication parameter.
     * @tparam P The CommParam enumerator.
     * @param value The value to set.
     */
    template <CommParam P>
    void setParam(const ParamCppT<paramType(P)>& value) {
        std::string cmd =
            "WN," + std::to_string(static_cast<int>(P)) + "," + formatParam<paramType(P)>(value);
        comm.sendCommand(cmd);
    }

   private:
    ICommInterface& comm;
    std::string model, firmwareVersion, macAddress;
};

}  // namespace OpenSRX
