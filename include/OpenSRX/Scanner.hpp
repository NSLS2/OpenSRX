#pragma once

#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/ICommInterface.hpp"
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


class Scanner {
public:
Scanner(ICommInterface& comm);
~Scanner() = default;

std::tuple<std::string, std::string> getVersionInfo();
// std::string getMacAddress();

// CommandStatus getCommandStatus();
// ErrorStatus getErrorStatus();
// BusyStatus getBusyStatus();

// std::string read();
// std::string read(int bank);
// void stopReading();

// Timestamp getTime();
// void setTime(const Timestamp& timestamp);


private:
ICommInterface& comm;

};

}  // namespace OpenSRX