#pragma once

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace OpenSRX {

/**
 * @brief Raw decoded image data.
 *
 * Pixels are stored in row-major order, bottom-to-top (BMP native order is
 * flipped so that row 0 is the top of the image). Each pixel is `channels`
 * bytes wide (1 = grayscale, 3 = BGR, 4 = BGRA).
 */
struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;           ///< bytes per pixel (1, 3, or 4)
    std::vector<uint8_t> data;  ///< raw pixel buffer (width * height * channels)
};

/**
 * @brief Decode a BMP file into an Image.
 *
 * Supports uncompressed 8-bit (grayscale palette), 24-bit, and 32-bit BMPs.
 *
 * @param path Filesystem path to the .bmp file.
 * @return The decoded Image.
 * @throws std::runtime_error on I/O or format errors.
 */
Image decodeBMP(const std::string& path);

/**
 * @brief Decode a BMP from an in-memory byte buffer.
 *
 * @param data The raw BMP file bytes.
 * @return The decoded Image.
 * @throws std::runtime_error on format errors.
 */
Image decodeBMPFromMemory(const std::vector<uint8_t>& data);

}  // namespace OpenSRX
