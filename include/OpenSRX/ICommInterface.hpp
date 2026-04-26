#pragma once
#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <mutex>
#include <string>

class MockCommInterface;

namespace OpenSRX {

enum class ErrCode {
    // Undefined command received.
    CMD_UNDEFINED = 0,

    // Mismatched command format (invalid number of parameters).
    MISMATCHED_CMD_FMT = 1,

    // The parameter 1 value exceeds the set value.
    PARAM1_OUT_OF_RANGE = 2,

    // The parameter 2 value exceeds the set value.
    PARAM2_OUT_OF_RANGE = 3,

    // Parameter 2 is not set in HEX (hexadecimal) code.
    PARAM2_NOT_IN_HEX = 4,

    // Parameter 2 is set in HEX (hexadecimal) code but exceeds the set value.
    PARAM2_IN_HEX_BUT_OUT_OF_RANGE = 5,

    // There are 2 or more ! marks in the preset data; preset data is incorrect.
    TWO_OR_MORE_MARKS_IN_PRESET_DATA = 10,

    // Area specification data is incorrect.
    AREA_SPECIFICATION_DATA_INCORRECT = 11,

    // Specified file does not exist.
    FILE_DOES_NOT_EXIST = 12,

    // "mm" for the %Tmm-LON,bb command exceeds the setting range.
    TMM_LON_MM_OUT_OF_RANGE = 13,

    // Communication cannot be checked with the %Tmm-KEYENCE command.
    TMM_KEYENCE_COMMUNICATION_CANNOT_BE_CHECKED = 14,

    // This command is not executable in the current status (execution error).
    COMMAND_NOT_EXECUTABLE_IN_CURRENT_STATUS = 20,

    // The buffer has overflowed, so commands cannot be executed.
    BUFFER_OVERFLOW = 21,

    // An error occurred while loading or saving parameters, so commands cannot be executed.
    PARAMETER_LOAD_OR_SAVE_ERROR = 22,

    // Commands cannot be received because the reader is connected to AutoID Network Navigator.
    CONNECTED_TO_AUTOID_NETWORK_NAVIGATOR = 23,

    // SR-X Series may be faulty; contact your nearest KEYENCE sales office.
    DEVICE_FAULT = 99,

};

enum class CommFormat {
    NO_HEADER_CR_IN_CR_OUT = 0,
    NO_HEADER_CRLF_IN_CR_OUT = 1,
    STX_HEADER_ETX_IN_ETX_OUT = 2,
};

class ICommInterface {
   public:
    template <typename StreamT>
    friend class AsioInterface;

    // This is not very nice, but we want to be able to test the helper utility functions
    friend class MockCommInterface;

    virtual ~ICommInterface() = default;
    virtual std::string sendCommand(const std::string& command) = 0;
    virtual std::string describe() const = 0;
    void setCommFormat(CommFormat format);
    CommFormat getCommFormat() { return commFormat; }

   private:
    std::string stripEcho(const std::string& response, const std::string& command);
    std::string addHeaderAndTerminator(const std::string& command);
    CommFormat commFormat = CommFormat::NO_HEADER_CR_IN_CR_OUT;
    std::string inTermStr = "\r";

   protected:
    /// Mutex to ensure thread-safe access to the communication interface.
    std::mutex commMutex;
};
}  // namespace OpenSRX
