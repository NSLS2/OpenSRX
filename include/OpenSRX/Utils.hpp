#pragma once

#include <string>
#include <tuple>

std::string stripEcho(const std::string& response, const std::string& command);
std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw);