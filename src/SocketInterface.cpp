#include "OpenSRX/SocketInterface.hpp"
#include <asio.hpp>

namespace OpenSRX {

SocketInterface::SocketInterface(std::string ip, int port) : socket(ioContext) {
    asio::ip::tcp::resolver resolver(ioContext);
    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, std::to_string(port));
    asio::connect(socket, endpoints);
}

SocketInterface::~SocketInterface() {
    socket.close();
}



};  // namespace OpenSRX