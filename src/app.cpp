#include <tuple>

#include "OpenSRX/API.hpp"
#include "OpenSRX/version.hpp"

namespace OpenSRX {

std::string app_name() { return "OpenSRX"; }

template <>
std::string GetVersion<std::string>() {
    return std::string(versionString);
}

template <>
VersionTuple GetVersion<VersionTuple>() {
    return std::make_tuple(versionMajor, versionMinor, versionPatch);
}

}  // namespace OpenSRX
