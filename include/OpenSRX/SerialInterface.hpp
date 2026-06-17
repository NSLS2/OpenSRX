#pragma once

#include <memory>

#include "OpenSRX/ICommInterface.hpp"

namespace OpenSRX {

struct SerialInterfaceImpl;

/// Serial port parity setting.
enum class Parity {
    NONE = 0,  ///< No parity.
    EVEN = 1,  ///< Even parity.
    ODD = 2,   ///< Odd parity.
};

/// Serial port data bits setting.
enum class DataBits {
    SEVEN = 7,  ///< 7 data bits.
    EIGHT = 8,  ///< 8 data bits.
};

/// Serial port stop bits setting.
enum class StopBits {
    ONE = 1,  ///< 1 stop bit.
    TWO = 2,  ///< 2 stop bits.
};

/// Serial port flow control setting.
enum class FlowControl {
    NONE = 0,      ///< No flow control.
    RTS_CTS = 1,   ///< Hardware (RTS/CTS) flow control.
    XON_XOFF = 2,  ///< Software (XON/XOFF) flow control.
};

/**
 * @brief Communication interface using an RS-232 serial port.
 *
 * Wraps an Asio serial_port with configurable baud rate, data bits,
 * parity, stop bits, and flow control.
 */
class SerialInterface : public ICommInterface {
   public:
    /**
     * @brief Open a serial port connection.
     *
     * @param port        Serial device path (e.g. "/dev/ttyUSB0").
     * @param baudRate    Baud rate (e.g. 115200).
     * @param dataBits    Number of data bits per character.
     * @param parity      Parity checking mode.
     * @param stopBits    Number of stop bits.
     * @param flowControl Flow control mode.
     */
    SerialInterface(std::string port = "/dev/ttyUSB0", int baudRate = 115200,
                    DataBits dataBits = DataBits::EIGHT, Parity parity = Parity::EVEN,
                    StopBits stopBits = StopBits::ONE, FlowControl flowControl = FlowControl::NONE);
    ~SerialInterface() override;

    /// @copydoc ICommInterface::describe()
    std::string describe() const override { return port; }

   private:
    std::string port;  ///< Serial device path.
    std::unique_ptr<SerialInterfaceImpl> impl;
};

};  // namespace OpenSRX
