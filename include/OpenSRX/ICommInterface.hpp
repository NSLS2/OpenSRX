#pragma once

#include <memory>
#include <mutex>
#include <string>

class MockCommInterface;

namespace OpenSRX {
namespace detail {
class IWireTransport;
}  // namespace detail

/// Error codes returned by the scanner in "ER,CMD,code" responses.
enum class ErrCode {
    CMD_UNDEFINED = 0,                   ///< Undefined command received.
    MISMATCHED_CMD_FMT = 1,              ///< Mismatched command format (invalid parameter count).
    PARAM1_OUT_OF_RANGE = 2,             ///< Parameter 1 value exceeds the set range.
    PARAM2_OUT_OF_RANGE = 3,             ///< Parameter 2 value exceeds the set range.
    PARAM2_NOT_IN_HEX = 4,               ///< Parameter 2 is not in hexadecimal.
    PARAM2_IN_HEX_BUT_OUT_OF_RANGE = 5,  ///< Parameter 2 is hex but out of range.
    TWO_OR_MORE_MARKS_IN_PRESET_DATA = 10,   ///< Preset data contains two or more "!" marks.
    AREA_SPECIFICATION_DATA_INCORRECT = 11,  ///< Area specification data is incorrect.
    FILE_DOES_NOT_EXIST = 12,                ///< Specified file does not exist.
    TMM_LON_MM_OUT_OF_RANGE = 13,            ///< TMM-LON mm value out of range.
    TMM_KEYENCE_COMMUNICATION_CANNOT_BE_CHECKED = 14,  ///< TMM-KEYENCE communication check failed.
    COMMAND_NOT_EXECUTABLE_IN_CURRENT_STATUS = 20,  ///< Command not executable in current status.
    BUFFER_OVERFLOW = 21,                           ///< Buffer overflow; commands cannot execute.
    PARAMETER_LOAD_OR_SAVE_ERROR = 22,              ///< Parameter load/save error.
    CONNECTED_TO_AUTOID_NETWORK_NAVIGATOR = 23,     ///< Connected to AutoID Network Navigator.
    DEVICE_FAULT = 99,                              ///< Device fault; contact KEYENCE support.
};

/// Communication framing format for command/response exchange.
enum class CommFormat {
    NO_HEADER_CR_IN_CR_OUT = 0,     ///< No header; CR terminator in both directions.
    NO_HEADER_CRLF_IN_CR_OUT = 1,   ///< No header; CR+LF in, CR out.
    STX_HEADER_ETX_IN_ETX_OUT = 2,  ///< STX header with ETX terminator.
};

/**
 * @brief Abstract interface for communicating with a scanner.
 *
 * Provides send/receive primitives and framing configuration. Concrete
 * implementations include SerialInterface, SocketInterface, and test mocks.
 */
class ICommInterface {
   public:
    friend class MockCommInterface;

    virtual ~ICommInterface();

    /**
     * @brief Send a command and return the response (thread-safe).
     *
     * Acquires the communication mutex before sending.
     *
     * @param command The command string (without header/terminator).
     * @return The parsed response string.
     */
    virtual std::string sendCommand(const std::string& command);

    /**
     * @brief Send a command and return the response without locking.
     *
     * Used by blocking commands (LON, TUNE, etc.) so that cancellation
     * commands can be sent concurrently from another thread.
     *
     * @param command The command string (without header/terminator).
     * @return The parsed response string.
     */
    virtual std::string sendCommandUnlocked(const std::string& command);

    /**
     * @brief Return a human-readable description of this interface.
     * @return e.g. "/dev/ttyUSB0" or "192.168.1.100:9004".
     */
    virtual std::string describe() const = 0;

    /**
     * @brief Return the host/IP of the connected scanner.
     * @return The scanner's IP address or hostname.
     * @throws std::runtime_error if the interface has no network host.
     */
    virtual std::string getHost() const {
        throw std::runtime_error("getHost() not supported on this interface");
    }

    /**
     * @brief Set the communication framing format.
     * @param format The CommFormat to use.
     */
    void setCommFormat(CommFormat format);

    /**
     * @brief Get the current communication framing format.
     * @return The active CommFormat.
     */
    CommFormat getCommFormat() { return commFormat; }

   protected:
    /// The underlying communication channel for raw byte I/O.
    std::unique_ptr<detail::IWireTransport> wire;

    /**
     * @brief Strip a command echo from the start of a response.
     * @param response The raw response string.
     * @param command The command that was sent.
     * @return The response with any leading echo removed.
     */
    std::string stripEcho(const std::string& response, const std::string& command);

    /**
     * @brief Wrap a command string with the appropriate header and terminator.
     * @param command The bare command string.
     * @return The framed command ready to send on the wire.
     */
    std::string addHeaderAndTerminator(const std::string& command);

    CommFormat commFormat = CommFormat::NO_HEADER_CR_IN_CR_OUT;  ///< Active framing format.
    std::string inTermStr = "\r";  ///< Expected response terminator string.

    /// Mutex to ensure thread-safe access to the communication interface.
    std::mutex commMutex;
};
}  // namespace OpenSRX
