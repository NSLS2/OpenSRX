#include "OpenSRX/SocketInterface.hpp"

#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "OpenSRX/WireTransport.hpp"

namespace OpenSRX {

class SocketWireTransport : public detail::AsioWireTransport<asio::ip::tcp::socket> {
   public:
    SocketWireTransport() : AsioWireTransport(ioContext) {}

    void connectTo(const std::string& ip, int port) {
        asio::ip::tcp::resolver resolver(ioContext);
        asio::ip::tcp::resolver::results_type endpoints =
            resolver.resolve(ip, std::to_string(port));
        asio::connect(stream, endpoints);
    }

    void close() { stream.close(); }
};

struct SocketInterfaceImpl {
    SocketWireTransport* wirePtr = nullptr;
};

SocketInterface::SocketInterface(const std::string& ip, int port)
    : ip(ip), port(port), impl(std::make_unique<SocketInterfaceImpl>()) {
    auto transport = std::make_unique<SocketWireTransport>();
    impl->wirePtr = transport.get();
    wire = std::move(transport);

    spdlog::debug("Initializing socket connection to {}...", describe());
    impl->wirePtr->connectTo(this->ip, this->port);
    spdlog::debug("Socket connection established.");
}

SocketInterface::~SocketInterface() {
    spdlog::debug("Closing socket connection to {}...", describe());
    if (impl && impl->wirePtr) impl->wirePtr->close();
    spdlog::debug("Socket connection to {} closed.", describe());
}

};  // namespace OpenSRX
