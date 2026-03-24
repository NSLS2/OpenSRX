#include <iostream>

#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/Scanner.hpp"
#include "OpenSRX/SerialInterface.hpp"

int main() {
    std::cout << "OpenSRX " << OpenSRX::GetVersion<std::string>() << std::endl;
    OpenSRX::SerialInterface serial("/dev/ttyUSB0");
    OpenSRX::Scanner scanner(serial);
    auto versionInfo = scanner.getVersionInfo();
    return 0;
}
