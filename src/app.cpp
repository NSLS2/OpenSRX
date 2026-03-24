#include <tuple>

#include "OpenSRX/OpenSRX.hpp"
#include "OpenSRX/Version.hpp"

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
