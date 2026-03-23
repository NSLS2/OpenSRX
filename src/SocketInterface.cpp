#include "OpenSRX/SocketInterface.hpp"
#include <asio.hpp>

namespace OpenSRX {

bool SocketInterface::connect() {
    asio::io_context context;
    asio::ip::tcp::resolver resolver(context);
    this->socket = asio::ip::tcp::socket(ioContext);

    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, std::to_string(port));
    asio::connect(socket, endpoints);
}

};  // namespace OpenSRX