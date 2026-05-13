#pragma once

#include "OpenSRX/ICommInterface.hpp"

namespace OpenSRX {

/**
 * @brief Templated base class for Asio-based communication interfaces.
 *
 * Provides the shared sendCommand implementation for any Asio stream type
 * (e.g. asio::ip::tcp::socket, asio::serial_port). Subclasses only need to
 * construct the stream and implement describe().
 *
 * @tparam StreamT An Asio SyncReadStream & SyncWriteStream type.
 */
template <typename StreamT>
class AsioInterface : public ICommInterface {
   public:
    ~AsioInterface() override = default;

    /**
     * @brief Sends a command over the Asio stream and reads the response.
     * 
     * This method acquires the communication mutex before sending, so it can be
     * safely called from multiple threads. For blocking commands, sendCommandUnlocked()
     * can be used to allow concurrent cancellation commands from another thread.
     *
     * @param command The command string to send (without header/terminator).
     * @return The parsed response string.
     */
    std::string sendCommand(const std::string& command) override;

    /**
     * @brief Sends a command without acquiring the communication mutex.
     *
     * @param command The command string to send (without header/terminator).
     * @return The parsed response string.
     */
    std::string sendCommandUnlocked(const std::string& command) override;

   protected:
    asio::io_context ioContext;
    StreamT stream;

    /**
     * @brief Construct an AsioInterface, forwarding arguments to the stream.
     *
     * @tparam Args Argument types forwarded to the StreamT constructor.
     * @param args Arguments forwarded to the StreamT constructor.
     */
    template <typename... Args>
    explicit AsioInterface(Args&&... args) : stream(std::forward<Args>(args)...) {}
};

}  // namespace OpenSRX
