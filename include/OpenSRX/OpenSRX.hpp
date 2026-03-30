#pragma once

#include <spdlog/spdlog.h>

#include <string>
#include <tuple>

namespace OpenSRX {

using VersionTuple = std::tuple<int, int, int>;

template <typename T>
T GetVersion() = delete;

template <>
std::string GetVersion<std::string>();

template <>
VersionTuple GetVersion<VersionTuple>();

}  // namespace OpenSRX
