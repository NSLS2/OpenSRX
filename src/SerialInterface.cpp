
#include "OpenSRX/SerialInterface.hpp"

namespace OpenSRX {

SerialInterface::SerialInterface(std::string port, int baudRate, DataBits dataBits, Parity parity,
                                 StopBits stopBits, FlowControl flowControl)
    : port(port), serial(ioContext, port) {
    // Configure 115200 / 8 / E / 1
    serial.set_option(asio::serial_port_base::baud_rate(baudRate));
    serial.set_option(asio::serial_port_base::character_size(8));
    serial.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::even));
    serial.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    serial.set_option(
        asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
}

SerialInterface::~SerialInterface() { serial.close(); }

std::string SerialInterface::sendCommand(const std::string& command) {
    std::string fullCommand = addHeaderAndTerminator(command);

    asio::write(serial, asio::buffer(fullCommand));

    // Read from serial until we see our terminator
    asio::streambuf response;
    asio::read_until(serial, response, inTermStr);

    // Convert response to a string
    std::string result{asio::buffers_begin(response.data()), asio::buffers_end(response.data())};

    // Strip the terminator from the end of the response, and the header from the start if
    // applicable
    auto pos = result.find(inTermStr);
    if (pos != std::string::npos) result.erase(pos);

    if (commFormat == CommFormat::STX_HEADER_ETX_IN_ETX_OUT && !result.empty() &&
        result[0] == '\x02')
        result.erase(0, 1);

    return result;
}

};  // namespace OpenSRX
