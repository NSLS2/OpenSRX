#pragma once

#include <string>
#include <tuple>

/**
 * @brief Strip a command echo prefix from a response string.
 * @param response The raw response from the device.
 * @param command  The command that was sent (potential echo).
 * @return The response with any leading echo removed.
 */
std::string stripEcho(const std::string& response, const std::string& command);

/**
 * @brief Parse a raw version-info string ("MODEL,FIRMWARE") into components.
 * @param raw The raw string from the KEYENCE command.
 * @return A tuple of (model, firmwareVersion).
 * @throws std::runtime_error if the format is unexpected.
 */
std::tuple<std::string, std::string> parseVersionInfo(const std::string& raw);
