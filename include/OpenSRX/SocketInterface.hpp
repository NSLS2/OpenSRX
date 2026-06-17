#pragma once

#include <memory>
#include <thread>

#include "OpenSRX/AsioInterface.hpp"

namespace OpenSRX {

struct SocketInterfaceImpl;

/**
 * @brief Communication interface using a TCP/IP socket.
 *
 * Connects to the scanner's Ethernet command port via Asio TCP socket.
 */
class SocketInterface : public AsioInterface<asio::ip::tcp::socket> {
   public:
    /**
     * @brief Open a TCP socket connection to the scanner.
     *
     * @param ip   Scanner IP address (e.g. "192.168.100.100").
     * @param port Scanner command port (default 9004).
     */
    SocketInterface(const std::string& ip = "192.168.100.100", int port = 9004);
    ~SocketInterface() override;

    /// @copydoc ICommInterface::describe()
    std::string describe() const override { return ip + ":" + std::to_string(port); };

    /**
     * @brief Start a local FTP server for receiving data from the scanner.
     *
     * @param address        Local address to bind to.
     * @param port           FTP port to listen on.
     * @param mountPoint     Local filesystem path to serve.
     * @param threadPoolSize Number of FTP server threads.
     */
    void startFtpServer(const std::string& address, int port, const std::string& mountPoint,
                        int threadPoolSize = 4);

   private:
    std::string ip;  ///< Scanner IP address.
    int port;        ///< Scanner command port.

    /// Private implementation holding optional FTP server state.
    std::unique_ptr<SocketInterfaceImpl> impl;
};

}  // namespace OpenSRX
