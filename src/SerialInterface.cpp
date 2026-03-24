
#include <iostream>
#include <string>
#include <asio.hpp>
#include "OpenSRX/SerialInterface.hpp"

namespace OpenSRX {

SerialInterface::SerialInterface(std::string port, int baudRate, DataBits dataBits, Parity parity, StopBits stopBits, FlowControl flowControl)
    : serial(ioContext, port) {

        // Configure 115200 / 8 / E / 1
        serial.set_option(asio::serial_port_base::baud_rate(baudRate));
        serial.set_option(asio::serial_port_base::character_size(8));
        serial.set_option(asio::serial_port_base::parity(
            asio::serial_port_base::parity::even));
        serial.set_option(asio::serial_port_base::stop_bits(
            asio::serial_port_base::stop_bits::one));
        serial.set_option(asio::serial_port_base::flow_control(
            asio::serial_port_base::flow_control::none));
}

SerialInterface::~SerialInterface() {
    serial.close();
}

std::string SerialInterface::sendCommand(const std::string& command) {
    asio::write(serial, asio::buffer(command));
    asio::streambuf buf;
    // read_until will read up to and including the delimiter
    asio::read_until(serial, buf, '\r');
    std::istream is(&buf);
    std::string line;
    std::getline(is, line, '\r');
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }

    // Check if line startswith "OK" to indicate successful command execution
    if (line.rfind("OK", 0) == 0) {
        line = line.substr(2); // Remove "OK" prefix
        // Next we should get a 

    } else {
        throw std::runtime_error("Command execution failed: " + line);
    }

    return line;
}

};  // namespace OpenSRX