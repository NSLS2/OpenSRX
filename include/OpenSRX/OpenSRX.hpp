#pragma once

#include <string>
#include <tuple>

namespace OpenSRX {

using VersionTuple = std::tuple<int, int, int>;

std::string app_name();

template <typename T>
T GetVersion() = delete;

template <>
std::string GetVersion<std::string>();

template <>
VersionTuple GetVersion<VersionTuple>();



} // namespace OpenSRX