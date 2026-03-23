#pragma once

#include "OpenSRX/ICommInterface.hpp"
#include <asio.hpp>

namespace OpenSRX {

class SocketInterface : public ICommInterface {
public:
    SocketInterface(std::string ip = "192.168.100.100", int port = 9004) : ip(std::move(ip)), port(port) {};
    ~SocketInterface() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;
    bool sendCommand(const std::string& command) override;

private:

    std::string ip;
    int port;
    asio::io_context ioContext;
    asio::ip::tcp::socket socket;
};

}  // namespace OpenSRX