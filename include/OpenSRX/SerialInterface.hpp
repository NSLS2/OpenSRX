#pragma once

#include <asio.hpp>
#include "OpenSRX/ICommInterface.hpp"

namespace OpenSRX {

enum class Parity {
    NONE = asio::serial_port_base::parity::none,
    EVEN = asio::serial_port_base::parity::even,
    ODD = asio::serial_port_base::parity::odd,
};

enum class DataBits {
    SEVEN = 7,
    EIGHT = 8,
};

enum class StopBits {
    ONE = asio::serial_port_base::stop_bits::one,
    TWO = asio::serial_port_base::stop_bits::two,
};

enum class FlowControl {
    NONE = asio::serial_port_base::flow_control::none,
    RTS_CTS = asio::serial_port_base::flow_control::hardware,
    XON_XOFF = asio::serial_port_base::flow_control::software,
};

class SerialInterface : public ICommInterface {
public:
    SerialInterface(std::string port = "/dev/ttyUSB0", int baudRate = 115200, DataBits dataBits = DataBits::EIGHT, Parity parity = Parity::EVEN, StopBits stopBits = StopBits::ONE, FlowControl flowControl = FlowControl::NONE);
    ~SerialInterface() override;

    std::string sendCommand(const std::string& command) override;

private:

    asio::io_context ioContext;
    asio::serial_port serial;
};

};  // namespace OpenSRX