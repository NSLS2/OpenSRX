#pragma once

#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "OpenSRX/ICommInterface.hpp"
#include "OpenSRX/ImageServer.hpp"
#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/ParamTraits.hpp"
#include "OpenSRX/Timestamp.hpp"

namespace OpenSRX {

class TestScanner;

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

/**
 * @brief Configuration for which image types to save via FTP.
 *
 * Each flag, when true, sets the corresponding SAVE_DEST parameter to
 * SEND_BY_FTP. When false the parameter is left unchanged.
 */
struct ImageSaveConfig {
    bool readOK = true;            ///< SAVE_DEST_READ_OK (500)
    bool verificationNG = false;   ///< SAVE_DEST_VERIFICATION_NG (501)
    bool readError = false;        ///< SAVE_DEST_READ_ERROR (502)
    bool unstable = false;         ///< SAVE_DEST_UNSTABLE (503)
    bool capture = false;          ///< SAVE_DEST_CAPTURE (504)
};

class Scanner {
   public:
    Scanner(ICommInterface& comm);
    ~Scanner() = default;

    // Allow test fixture access to private methods
    friend class TestScanner;

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

    // ─── Image server ───────────────────────────────────────────────────────

    /**
     * @brief Start an embedded FTP server and configure the scanner to send
     *        images to it.
     *
     * The server listens on a free port by default. The scanner's FTP IP,
     * username, password, and port parameters are written automatically.
     * Image saving destinations selected in @p saveConfig are set to
     * SEND_BY_FTP; they are restored to their previous values when
     * stopImageServer() is called.
     *
     * @param localIP     The IP address of this machine as reachable from the
     *                    scanner (e.g. "192.168.1.100").
     * @param saveConfig  Which image types to send via FTP (default: read OK only).
     * @param port        FTP port (0 = OS picks a free port).
     * @param username    FTP username.
     * @param password    FTP password.
     */
    void startImageServer(const std::string& localIP,
                          ImageSaveConfig saveConfig = {},
                          uint16_t port = 0,
                          const std::string& username = "opensrx",
                          const std::string& password = "opensrx");

    /** Stop the embedded FTP image server. */
    void stopImageServer();

    /**
     * @brief Block until the scanner delivers an image via FTP, decode it,
     *        and return the raw pixel data.
     *
     * Requires startImageServer() to have been called first.
     */
    Image waitForImage();

    /**
     * @brief Return the next image if one is available, without blocking.
     *
     * @param[out] image  Filled on success.
     * @return true if an image was available.
     */
    bool tryGetImage(Image& image);

    /**
     * @brief Return all images currently queued.
     */
    std::deque<Image> getImages();

    /**
     * @brief Set a callback invoked on the watcher thread each time a new
     *        image is decoded.
     */
    void setImageCallback(std::function<void(const Image&)> cb);

    /**
     * @brief Access the underlying ImageServer (nullptr if not started).
     */
    ImageServer* getImageServer() { return imageServer.get(); }

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
    std::string checkResponse(const std::string& response);
    ICommInterface& comm;
    std::string model, firmwareVersion, macAddress;
    std::unique_ptr<ImageServer> imageServer;

    /// Previous SAVE_DEST values saved by startImageServer, restored on stop.
    struct SavedImageDests {
        ImageSavingDestination readOK;
        ImageSavingDestination verificationNG;
        ImageSavingDestination readError;
        ImageSavingDestination unstable;
        ImageSavingDestination capture;
    };
    std::unique_ptr<SavedImageDests> savedImageDests;
};

}  // namespace OpenSRX
