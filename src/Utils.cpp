#include <OpenSRX/Utils.hpp>
#include <string>
#include <tuple>

std::string stripEcho(const std::string& response, const std::string& command) {
    // Strip the trailing \r from the command for prefix matching.
    std::string cmd = command;
    if (!cmd.empty() && cmd.back() == '\r') {
        cmd.pop_back();
    }

    if (response.rfind(cmd, 0) == 0) {
        auto pos = cmd.size();
        // Skip a comma separator between the echo and the payload.
        if (pos < response.size() && response[pos] == ',') {
            ++pos;
        }
        return response.substr(pos);
    }
    return response;
}


std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw) {
    
}