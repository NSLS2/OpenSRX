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
 * Pixels are stored in row-major order, top-to-bottom. Each pixel is
 * `channels` bytes wide (1 = grayscale, 3 = BGR, 4 = BGRA).
 *
 * When `compressed` is true, `data` holds the original JPEG file bytes
 * and `width`, `height`, `channels` reflect the image dimensions read
 * from the JPEG header (without decompressing the pixel data).
 */
struct Image {
    int width = 0;
    int height = 0;
    int channels = 0;           ///< bytes per pixel (1, 3, or 4)
    bool compressed = false;    ///< true if data holds compressed JPEG bytes
    std::vector<uint8_t> data;  ///< pixel buffer or compressed JPEG data
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
 * @brief Decode a JPEG file into an Image.
 *
 * @param path       Filesystem path to the .jpg/.jpeg file.
 * @param decompress If true (default), fully decompress the pixel data.
 *                   If false, `data` holds the raw JPEG file bytes and
 *                   `compressed` is set to true; `width`, `height`, and
 *                   `channels` are still populated from the JPEG header.
 * @return The decoded (or header-parsed) Image.
 * @throws std::runtime_error on I/O or format errors.
 */
Image decodeJPEG(const std::string& path, bool decompress = true);

}  // namespace OpenSRX
