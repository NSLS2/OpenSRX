#pragma once

#include <memory>
#include <thread>

#include "OpenSRX/ICommInterface.hpp"

namespace OpenSRX {

struct SocketInterfaceImpl;

/**
 * @brief Communication interface using a TCP/IP socket.
 *
 * Connects to the scanner's Ethernet command port via Asio TCP socket.
 */
class SocketInterface : public ICommInterface {
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

    /// @copydoc ICommInterface::getHost()
    std::string getHost() const override { return ip; }

   private:
    std::string ip;  ///< Scanner IP address.
    int port;        ///< Scanner command port.

    /// Private implementation holding optional FTP server state.
    std::unique_ptr<SocketInterfaceImpl> impl;
};

}  // namespace OpenSRX
