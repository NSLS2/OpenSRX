#include "OpenSRX/Image.hpp"

#include <cstring>

namespace OpenSRX {

// ── BMP header helpers (little-endian) ──────────────────────────────────────

namespace {

template <typename T>
T readLE(const uint8_t* p);

template <>
uint16_t readLE<uint16_t>(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

template <>
uint32_t readLE<uint32_t>(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

template <>
int32_t readLE<int32_t>(const uint8_t* p) {
    uint32_t u = readLE<uint32_t>(p);
    int32_t v;
    std::memcpy(&v, &u, sizeof(v));
    return v;
}

}  // namespace

Image decodeBMP(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open BMP file: " + path);

    // Read file header (14 bytes) + DIB header start (at least 40 bytes)
    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);
    if (!file || file.gcount() < 54)
        throw std::runtime_error("BMP file too small: " + path);

    if (header[0] != 'B' || header[1] != 'M')
        throw std::runtime_error("Not a BMP file: " + path);

    uint32_t pixelOffset = readLE<uint32_t>(header + 10);
    int32_t  width       = readLE<int32_t>(header + 18);
    int32_t  height      = readLE<int32_t>(header + 22);
    uint16_t bitsPerPx   = readLE<uint16_t>(header + 28);
    uint32_t compression = readLE<uint32_t>(header + 30);

    if (compression != 0)
        throw std::runtime_error("Compressed BMPs are not supported: " + path);

    // Handle top-down BMPs (negative height)
    bool topDown = height < 0;
    if (topDown) height = -height;

    if (width <= 0 || height <= 0)
        throw std::runtime_error("Invalid BMP dimensions: " + path);

    int channels;
    switch (bitsPerPx) {
        case 8:  channels = 1; break;
        case 24: channels = 3; break;
        case 32: channels = 4; break;
        default:
            throw std::runtime_error(
                "Unsupported BMP bit depth (" + std::to_string(bitsPerPx) + "): " + path);
    }

    // Each row is padded to a multiple of 4 bytes
    int rowBytes = width * (bitsPerPx / 8);
    int rowPadding = (4 - (rowBytes % 4)) % 4;
    int paddedRowBytes = rowBytes + rowPadding;

    Image img;
    img.width = width;
    img.height = height;
    img.channels = channels;
    img.data.resize(static_cast<size_t>(width) * height * channels);

    std::vector<uint8_t> rowBuf(paddedRowBytes);

    file.seekg(pixelOffset);

    for (int y = 0; y < height; ++y) {
        file.read(reinterpret_cast<char*>(rowBuf.data()), paddedRowBytes);
        if (!file)
            throw std::runtime_error("Unexpected end of BMP pixel data: " + path);

        // BMP stores rows bottom-to-top by default; flip so row 0 = top.
        int destRow = topDown ? y : (height - 1 - y);
        std::memcpy(img.data.data() + static_cast<size_t>(destRow) * rowBytes,
                     rowBuf.data(), rowBytes);
    }

    return img;
}

}  // namespace OpenSRX
