#pragma once

#include <spdlog/spdlog.h>

#include <string>
#include <tuple>

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
