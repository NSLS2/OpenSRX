#include "OpenSRX/SocketInterface.hpp"

#include <asio.hpp>
#include <fineftp/server.h>
#include <spdlog/spdlog.h>

#include "OpenSRX/WireTransport.hpp"

namespace OpenSRX {

class SocketWireTransport : public detail::AsioWireTransport<asio::ip::tcp::socket> {
   public:
    SocketWireTransport() : AsioWireTransport(ioContext) {}

    void connectTo(const std::string& ip, int port) {
        asio::ip::tcp::resolver resolver(ioContext);
        asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, std::to_string(port));
        asio::connect(stream, endpoints);
    }

    void close() { stream.close(); }
};

struct SocketInterfaceImpl {
    SocketWireTransport* wirePtr = nullptr;
    std::unique_ptr<fineftp::FtpServer> ftpServer;
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
    if (impl->ftpServer != nullptr && impl->ftpServer->getOpenConnectionCount() > 0) {
        spdlog::debug("Stopping FTP server with {} open connections...",
                      impl->ftpServer->getOpenConnectionCount());
        impl->ftpServer->stop();
        spdlog::debug("FTP server stopped.");
    }
    spdlog::debug("Socket connection to {} closed.", describe());
}

void SocketInterface::startFtpServer(const std::string& address, int port,
                                     const std::string& mountPoint, int threadPoolSize) {
    spdlog::debug("Starting FTP server on {}:{} with mount point '{}'...", address, port,
                  mountPoint);
    impl->ftpServer = std::make_unique<fineftp::FtpServer>(address, port);
    impl->ftpServer->addUserAnonymous(mountPoint, fineftp::Permission::All);
    impl->ftpServer->start(threadPoolSize);
    spdlog::debug("FTP server started on {}:{}.", address, port);
}

};  // namespace OpenSRX
