#pragma once

#include "OpenSRX/ICommInterface.hpp"
#include <asio.hpp>

namespace OpenSRX {

class SocketInterface : public ICommInterface {
public:
    SocketInterface(std::string ip = "192.168.100.100", int port = 9004);
    ~SocketInterface() override;

    // bool sendCommand(const std::string& command) override;

private:
    asio::io_context ioContext;
    asio::ip::tcp::socket socket;
};

}  // namespace OpenSRX