#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "OpenSRX/Code.hpp"
#include "OpenSRX/ICommInterface.hpp"
#include "OpenSRX/Image.hpp"
#include "OpenSRX/ParamTraits.hpp"
#include "OpenSRX/Timestamp.hpp"

namespace OpenSRX {

class TestScanner;

/// Scanner command processing status.
enum class CommandStatus {
    NO_PROCESSING = 0,     ///< No command is being processed.
    WAIT_FOR_SETTING = 1,  ///< Waiting for a setting to be applied.
    UPDATING = 2,          ///< A setting update is in progress.
};

/// Scanner error status codes.
enum class ErrorStatus {
    NO_ERROR = 0,             ///< No error.
    SYSTEM_ERROR = 1,         ///< System error.
    UPDATE_ERROR = 2,         ///< Update error.
    SET_VALUE_ERROR = 3,      ///< Set value error.
    DUPLICATE_IP_ERROR = 4,   ///< Duplicate IP address detected.
    BUFF_OVERFLOW_ERROR = 5,  ///< Buffer overflow error.
    PLC_LINK_ERROR = 6,       ///< PLC link error.
    PROFINET_ERROR = 7,       ///< PROFINET error.
    LUA_SCRIPT_ERROR = 8,     ///< Lua script error.
    CONNECTION_ERROR = 9,     ///< Host connection error.
};

/// Scanner busy status.
enum class BusyStatus {
    IDLE = 0,               ///< Not busy.
    TRG_BUSY = 1,           ///< Trigger processing in progress.
    UPDATE_PROCESSING = 2,  ///< Update processing in progress.
    SAVING_FILE = 3,        ///< Saving a file.
    AUTO_FOCUSING = 4,      ///< Auto-focus in progress.
};

/// Advice returned by tuning on how to improve results.
enum class TuningAdvice {
    NONE = 0,                 ///< No advice.
    USE_AN_IMAGE_FILTER = 1,  ///< Try using an image filter.
    CONSIDER_INSTALLATION_LIGHTING_PRINTING_CONDITIONS =
        2,                        ///< Check installation/lighting/printing.
    BRIGHTNESS_INSUFFICIENT = 4,  ///< Brightness is insufficient.
};

/// Reason for a tuning failure.
enum class TuningFailureReason {
    CODE_DETECTION_IMPOSSIBLE = 1,  ///< Code detection is impossible.
    UNSTABLE_READING = 2,           ///< Reading is unstable.
};

/// @brief Parse a raw version info string ("MODEL,FIRMWARE") into components.
/// @param raw The raw string returned by the KEYENCE command.
/// @return A tuple of (model, firmwareVersion).
/// @throws std::runtime_error if the format is unexpected.
std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw);

/**
 * @brief High-level interface to a KEYENCE SR-X series barcode scanner.
 *
 * Wraps an ICommInterface and exposes typed methods for every scanner
 * command: reading, tuning, I/O control, status queries, settings
 * management, and parameter get/set. Also provides an integrated FTP
 * image server for receiving captured images.
 */
class Scanner {
   public:
    /**
     * @brief Connect to a scanner on the given communication interface.
     *
     * Sends the KEYENCE handshake and EMAC commands to obtain the model,
     * firmware version, and MAC address.
     *
     * @param comm A previously-opened communication interface.
     * @throws std::runtime_error on handshake failure.
     */
    Scanner(ICommInterface& comm);
    ~Scanner() = default;

    // Allow test fixture access to private methods
    friend class TestScanner;

    /**
     * @brief Start a blocking read operation.
     *
     * Blocks until a code is read or the read times out. Uses an unlocked
     * send so that stopReading() can be called from another thread.
     *
     * @return A Code struct with the decoded data and any appended metadata.
     * @throws std::runtime_error on read timeout or failure.
     */
    Code startReading();

    /**
     * @brief Start a blocking read operation on a specific bank.
     * @param bank Bank number (1–16).
     * @return A Code struct with the decoded data and any appended metadata.
     * @throws std::runtime_error on read timeout or failure.
     */
    Code startReading(int bank);

    /** @brief Cancel an in-progress read operation. */
    void stopReading();

    /** @brief Enter quick-setup code reading mode (RCON). */
    void startQuickSetupCodeReading();

    /** @brief Exit quick-setup code reading mode (RCOFF). */
    void finishQuickSetupCodeReading();

    /**
     * @brief Check the result of a quick-setup code reading.
     * @return The result data string.
     */
    std::string checkQuickSetupCodeResult();

    /** @brief Start a reading rate test on all banks. */
    void readingRateTest();

    /**
     * @brief Start a reading rate test on a specific bank.
     * @param bank Bank number (1–16).
     */
    void readingRateTest(int bank);

    /** @brief Start a read time test on all banks. */
    void readTimeTest();

    /**
     * @brief Start a read time test on a specific bank.
     * @param bank Bank number (1–16).
     */
    void readTimeTest(int bank);

    /** @brief Quit test mode (reading rate or read time). */
    void quitTestMode();

    /**
     * @brief Get the state of an input terminal.
     * @param terminalNumber Input terminal number.
     * @return true if the terminal is ON.
     */
    bool getInputTerminalState(int terminalNumber);

    /**
     * @brief Turn on a specific output terminal.
     * @param terminalNumber Output terminal number.
     */
    void turnOnOutputTerminal(int terminalNumber);

    /**
     * @brief Turn off a specific output terminal.
     * @param terminalNumber Output terminal number.
     */
    void turnOffOutputTerminal(int terminalNumber);

    /** @brief Turn on all output terminals. */
    void turnOnAllOutputTerminals();

    /** @brief Turn off all output terminals. */
    void turnOffAllOutputTerminals();

    /** @brief Reset the scanner. */
    void reset();

    /** @brief Clear the scanner's send buffer. */
    void clearSendBuffer();

    /**
     * @brief Capture an image and return the remote file path.
     * @param bank Bank number (1–16).
     * @return The path to the captured image file on the scanner.
     */
    std::string captureImage(int bank);

    /** @brief Run auto-focus adjustment (blocks until complete). */
    void adjustFocus();

    /**
     * @brief Start tuning on a specific bank (blocks until complete).
     *
     * Uses an unlocked send so that stopTuning() can be called from
     * another thread.
     *
     * @param bank Bank number (1–16).
     * @return A tuple of (succeeded, advice, failureReason).
     */
    std::tuple<bool, TuningAdvice, TuningFailureReason> startTuning(int bank);

    /** @brief Cancel an in-progress tuning operation. */
    void stopTuning();

    /** @brief Get the scanner model string (e.g. "SR-X300"). */
    std::string getModel() const { return model; }

    /** @brief Get the scanner firmware version string. */
    std::string getFirmwareVersion() const { return firmwareVersion; }

    /** @brief Get the scanner MAC address string. */
    std::string getMacAddress() const { return macAddress; }

    /** @brief Enable the laser aiming pointer. */
    void enablePointer();

    /** @brief Disable the laser aiming pointer. */
    void disablePointer();

    /**
     * @brief Set the scanner's internal clock.
     * @param timestamp The date/time to set.
     */
    void setTime(const Timestamp& timestamp);

    /**
     * @brief Read the scanner's internal clock.
     * @return The current Timestamp.
     */
    Timestamp getTime();

    /**
     * @brief Query the scanner's command processing status.
     * @return The current CommandStatus.
     */
    CommandStatus getCommandStatus();

    /**
     * @brief Query the scanner's error status.
     * @return The current ErrorStatus.
     */
    ErrorStatus getErrorStatus();

    /**
     * @brief Query the scanner's busy status.
     * @return The current BusyStatus.
     */
    BusyStatus getBusyStatus();

    /**
     * @brief Copy configuration from one bank to another.
     * @param sourceBank Source bank number (1–16).
     * @param targetBank Target bank number (1–16).
     */
    void copyBankConfiguration(int sourceBank, int targetBank);

    /** @brief Save all current settings to non-volatile memory. */
    void saveSettings();

    /** @brief Load settings from non-volatile memory. */
    void loadSavedSettings();

    /** @brief Reset all settings to factory defaults. */
    void resetToFactorySettings();

    /**
     * @brief Save settings to a backup slot.
     * @param backupNumber Backup slot number.
     */
    void saveBackupSettings(int backupNumber);

    /**
     * @brief Load settings from a backup slot.
     * @param backupNumber Backup slot number.
     */
    void loadBackupSettings(int backupNumber);

    /** @brief Clear the FTP communication error flag. */
    void clearFTPCommsError();

    /** @brief Clear the PLC link error flag. */
    void clearPLCLinkError();

    // ─── Image retrieval ────────────────────────────────────────────────────

    /**
     * @brief Download an image from the scanner's FTP server and decode it.
     *
     * The scanner runs an anonymous FTP server on port 21. This method
     * connects to it and retrieves the file at the given path.
     *
     * @param remotePath  Path on the scanner (e.g. "/IMAGE/001_C_01.BMP")
     *                    as returned by captureImage().
     * @return The decoded image.
     */
    Image fetchImage(const std::string& remotePath);

    /**
     * @brief Fetch the most recently saved image from the scanner.
     *
     * Lists the /IMAGE/ directory on the scanner's FTP server and downloads
     * the last (newest) file. Useful for retrieving the image taken during
     * a read operation without issuing a separate SHOT command.
     *
     * @return The decoded image.
     * @throws std::runtime_error if no images are found or connection fails.
     */
    Image fetchLatestImage();

    /**
     * @brief Capture a snapshot and return the decoded image.
     *
     * Sends SHOT to the scanner, then downloads the resulting image file
     * via the scanner's built-in FTP server.
     *
     * @param bank Bank number (1–16).
     * @return The decoded image.
     */
    Image captureSnapshot(int bank);

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
        std::string raw = checkResponse(comm.sendCommand(cmd.str()));
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
        checkResponse(comm.sendCommand(cmd.str()));
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
        std::string raw = checkResponse(comm.sendCommand(cmd));
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
        checkResponse(comm.sendCommand(cmd));
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
        std::string raw = checkResponse(comm.sendCommand(cmd));
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
        checkResponse(comm.sendCommand(cmd));
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
        std::string raw = checkResponse(comm.sendCommand(cmd));
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
        checkResponse(comm.sendCommand(cmd));
    }

   private:
    /**
     * @brief Parse a raw read-result string into a Code struct.
     *
     * Splits the result by the inter-delimiter and populates optional
     * fields based on which OperationParam appending settings are enabled.
     *
     * @param raw The raw string returned by LON (barcode + appended fields).
     * @return A populated Code struct.
     */
    Code parseReadResult(const std::string& raw);

    /**
     * @brief Parse and validate a scanner response string.
     *
     * Extracts the value from an "OK,CMD,value" response, or throws
     * on an "ER,CMD,code" error response.
     *
     * @param response The raw response string from the scanner.
     * @return The extracted value portion of the response.
     * @throws std::invalid_argument, std::out_of_range, std::runtime_error
     *         depending on the error code.
     */
    std::string checkResponse(const std::string& response);

    ICommInterface& comm;           ///< Communication interface to the scanner.
    std::string model;              ///< Scanner model string.
    std::string firmwareVersion;    ///< Scanner firmware version string.
    std::string macAddress;         ///< Scanner MAC address string.
};

}  // namespace OpenSRX
