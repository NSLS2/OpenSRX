#include "OpenSRX/AsioInterface.hpp"

namespace OpenSRX {

/**
 * @brief Sends a command over the Asio stream and reads the response.
 *
 * @param command The command string to send (without header/terminator).
 * @return The parsed response string.
 */
template <typename StreamT>
std::string AsioInterface<StreamT>::sendCommand(const std::string& command) {
    std::scoped_lock lock(commMutex);

    std::string fullCommand = addHeaderAndTerminator(command);
    asio::write(stream, asio::buffer(fullCommand));

    // Read from the stream until we see our terminator
    asio::streambuf response;
    asio::read_until(stream, response, inTermStr);

    // Convert response to a string
    std::string result{asio::buffers_begin(response.data()), asio::buffers_end(response.data())};

    // Strip the terminator from the end of the response, and the header
    // from the start if applicable
    auto pos = result.find(inTermStr);
    if (pos != std::string::npos) result.erase(pos);

    if (commFormat == CommFormat::STX_HEADER_ETX_IN_ETX_OUT && !result.empty() &&
        result[0] == '\x02')
        result.erase(0, 1);

    return result;
}

// Explicit instantiations for the two supported stream types.
template class AsioInterface<asio::ip::tcp::socket>;
template class AsioInterface<asio::serial_port>;

}  // namespace OpenSRX
