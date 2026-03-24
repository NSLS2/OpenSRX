#include "OpenSRX/Scanner.hpp"

namespace OpenSRX {




std::tuple<std::string, std::string> Scanner::getVersionInfo() {
    std::tuple<std::string, std::string> versionInfo;
    std::string raw = comm.sendCommand("KEYENCE\r");
    return versionInfo;
}

Scanner::Scanner(ICommInterface& comm) : comm(comm) {
    // TODO: Collect some inital information about the scanner, such as version info, mac address, etc.
}

};  // namespace OpenSRX