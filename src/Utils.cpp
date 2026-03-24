#include <OpenSRX/Utils.hpp>
#include <string>
#include <tuple>

std::string extractResponse(const std::string& response, const std::string& command) {
    if (response.rfind("OK", 0) == 0) {
        return response.substr(2); // Remove "OK" prefix
    } else {

    }

    if (response.rfind(command, 0) == 0) {
        return response.substr(command.size());
    }
    return response;
}


std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw) {
    
}