#include "OpenSRX/SerialInterface.hpp"

#include <spdlog/spdlog.h>

#include <asio.hpp>

#include "OpenSRX/WireTransport.hpp"

namespace OpenSRX {

namespace {

asio::serial_port_base::parity mapParity(Parity parity) {
    using AsioParityType = asio::serial_port_base::parity::type;
    switch (parity) {
        case Parity::NONE:
            return asio::serial_port_base::parity(AsioParityType::none);
        case Parity::EVEN:
            return asio::serial_port_base::parity(AsioParityType::even);
        case Parity::ODD:
            return asio::serial_port_base::parity(AsioParityType::odd);
    }
    return asio::serial_port_base::parity(AsioParityType::none);
}

asio::serial_port_base::stop_bits mapStopBits(StopBits stopBits) {
    using AsioStopBitsType = asio::serial_port_base::stop_bits::type;
    switch (stopBits) {
        case StopBits::ONE:
            return asio::serial_port_base::stop_bits(AsioStopBitsType::one);
        case StopBits::TWO:
            return asio::serial_port_base::stop_bits(AsioStopBitsType::two);
    }
    return asio::serial_port_base::stop_bits(AsioStopBitsType::one);
}

asio::serial_port_base::flow_control mapFlowControl(FlowControl flowControl) {
    using AsioFlowControlType = asio::serial_port_base::flow_control::type;
    switch (flowControl) {
        case FlowControl::NONE:
            return asio::serial_port_base::flow_control(AsioFlowControlType::none);
        case FlowControl::RTS_CTS:
            return asio::serial_port_base::flow_control(AsioFlowControlType::hardware);
        case FlowControl::XON_XOFF:
            return asio::serial_port_base::flow_control(AsioFlowControlType::software);
    }
    return asio::serial_port_base::flow_control(AsioFlowControlType::none);
}

}  // namespace

class SerialWireTransport : public detail::AsioWireTransport<asio::serial_port> {
   public:
    explicit SerialWireTransport(const std::string& port) : AsioWireTransport(ioContext, port) {}

    void configure(int baudRate, DataBits dataBits, Parity parity, StopBits stopBits,
                   FlowControl flowControl) {
        stream.set_option(asio::serial_port_base::baud_rate(baudRate));
        stream.set_option(
            asio::serial_port_base::character_size(static_cast<unsigned int>(dataBits)));
        stream.set_option(mapParity(parity));
        stream.set_option(mapStopBits(stopBits));
        stream.set_option(mapFlowControl(flowControl));
    }

    void close() { stream.close(); }
};

struct SerialInterfaceImpl {
    SerialWireTransport* wirePtr = nullptr;
};

SerialInterface::SerialInterface(std::string port, int baudRate, DataBits dataBits, Parity parity,
                                 StopBits stopBits, FlowControl flowControl)
    : port(std::move(port)), impl(std::make_unique<SerialInterfaceImpl>()) {
    auto transport = std::make_unique<SerialWireTransport>(this->port);
    impl->wirePtr = transport.get();
    wire = std::move(transport);

    spdlog::debug("Initializing serial connection to {}...", describe());
    spdlog::debug(
        "Serial port settings - Baud Rate: {}, Data Bits: {}, Parity: {}, Stop Bits: {}, Flow "
        "Control: {}",
        baudRate, static_cast<int>(dataBits), static_cast<int>(parity), static_cast<int>(stopBits),
        static_cast<int>(flowControl));
    impl->wirePtr->configure(baudRate, dataBits, parity, stopBits, flowControl);
    spdlog::debug("Serial connection to {} initialized.", describe());
}

SerialInterface::~SerialInterface() {
    spdlog::debug("Closing serial connection to {}...", describe());
    if (impl && impl->wirePtr) impl->wirePtr->close();
}

};  // namespace OpenSRX
