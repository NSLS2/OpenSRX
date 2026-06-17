
#include "OpenSRX/SerialInterface.hpp"

#include <spdlog/spdlog.h>

namespace OpenSRX {

SerialInterface::SerialInterface(std::string port, int baudRate, DataBits dataBits, Parity parity,
                                 StopBits stopBits, FlowControl flowControl)
    : AsioInterface(ioContext, port), port(port) {
    spdlog::debug("Initializing serial connection to {}...", describe());
    spdlog::debug(
        "Serial port settings - Baud Rate: {}, Data Bits: {}, Parity: {}, Stop Bits: {}, Flow "
        "Control: {}",
        baudRate, static_cast<int>(dataBits), static_cast<int>(parity), static_cast<int>(stopBits),
        static_cast<int>(flowControl));
    stream.set_option(asio::serial_port_base::baud_rate(baudRate));
    stream.set_option(asio::serial_port_base::character_size(8));
    stream.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::even));
    stream.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    stream.set_option(
        asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
    spdlog::debug("Serial connection to {} initialized.", describe());
}

SerialInterface::~SerialInterface() {
    spdlog::debug("Closing serial connection to {}...", describe());
    stream.close();
}

};  // namespace OpenSRX
