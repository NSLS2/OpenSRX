#pragma once
#include <string>

namespace OpenSRX {


enum class Header {
    NONE = 0,
    STX = 1,
};

enum class Terminator {
    CR = 0,
    CRLF = 1,
    ETX = 2,
};

class ICommInterface {
public:
    virtual ~ICommInterface() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual bool sendCommand(const std::string& command) = 0;


};
}  // namespace OpenSRX