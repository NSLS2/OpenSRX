#pragma once

/**
 * @file OpenSRX.hpp
 * @brief Umbrella header for the OpenSRX library.
 *
 * Include this single header to get access to the entire public API:
 * Scanner, communication interfaces, parameter types, image handling,
 * and version information.
 */

#include <string>
#include <tuple>

// ── Public API headers ──────────────────────────────────────────────────────
#include "OpenSRX/BankParam.hpp"
#include "OpenSRX/Code.hpp"
#include "OpenSRX/CommParam.hpp"
#include "OpenSRX/ICommInterface.hpp"
#include "OpenSRX/Image.hpp"
#include "OpenSRX/FtpClient.hpp"
#include "OpenSRX/OperationParam.hpp"
#include "OpenSRX/ParamTraits.hpp"
#include "OpenSRX/ParamValues.hpp"
#include "OpenSRX/RegionParam.hpp"
#include "OpenSRX/SerialInterface.hpp"
#include "OpenSRX/SocketInterface.hpp"
#include "OpenSRX/Timestamp.hpp"
#include "OpenSRX/TuningParam.hpp"

// Scanner is included last since it depends on the above headers.
#include "OpenSRX/Scanner.hpp"

namespace OpenSRX {

/// Tuple type for (major, minor, patch) version numbers.
using VersionTuple = std::tuple<int, int, int>;

/// @brief Get the library version (primary template is deleted).
template <typename T>
T GetVersion() = delete;

/**
 * @brief Get the library version as a human-readable string.
 * @return Version string, e.g. "1.2.3".
 */
template <>
std::string GetVersion<std::string>();

/**
 * @brief Get the library version as a (major, minor, patch) tuple.
 * @return VersionTuple with the three components.
 */
template <>
VersionTuple GetVersion<VersionTuple>();

}  // namespace OpenSRX
