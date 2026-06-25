#include "OpenSRX/Image.hpp"

#include <cstring>

#include <jpeglib.h>

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
    if (!file || file.gcount() < 54) throw std::runtime_error("BMP file too small: " + path);

    if (header[0] != 'B' || header[1] != 'M') throw std::runtime_error("Not a BMP file: " + path);

    uint32_t pixelOffset = readLE<uint32_t>(header + 10);
    int32_t width = readLE<int32_t>(header + 18);
    int32_t height = readLE<int32_t>(header + 22);
    uint16_t bitsPerPx = readLE<uint16_t>(header + 28);
    uint32_t compression = readLE<uint32_t>(header + 30);

    if (compression != 0) throw std::runtime_error("Compressed BMPs are not supported: " + path);

    // Handle top-down BMPs (negative height)
    bool topDown = height < 0;
    if (topDown) height = -height;

    if (width <= 0 || height <= 0) throw std::runtime_error("Invalid BMP dimensions: " + path);

    int channels;
    switch (bitsPerPx) {
        case 8:
            channels = 1;
            break;
        case 24:
            channels = 3;
            break;
        case 32:
            channels = 4;
            break;
        default:
            throw std::runtime_error("Unsupported BMP bit depth (" + std::to_string(bitsPerPx) +
                                     "): " + path);
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
        if (!file) throw std::runtime_error("Unexpected end of BMP pixel data: " + path);

        // BMP stores rows bottom-to-top by default; flip so row 0 = top.
        int destRow = topDown ? y : (height - 1 - y);
        std::memcpy(img.data.data() + static_cast<size_t>(destRow) * rowBytes, rowBuf.data(),
                    rowBytes);
    }

    return img;
}

Image decodeJPEG(const std::string& path, bool decompress) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open JPEG file: " + path);

    auto fileSize = file.tellg();
    file.seekg(0);
    std::vector<uint8_t> fileData(static_cast<size_t>(fileSize));
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    if (!file) throw std::runtime_error("Failed to read JPEG file: " + path);

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = [](j_common_ptr cinfo) {
        char msg[JMSG_LENGTH_MAX];
        cinfo->err->format_message(cinfo, msg);
        jpeg_destroy(cinfo);
        throw std::runtime_error(std::string("JPEG error: ") + msg);
    };

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, fileData.data(), fileData.size());

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        throw std::runtime_error("Invalid JPEG header: " + path);
    }

    Image img;
    img.width = static_cast<int>(cinfo.image_width);
    img.height = static_cast<int>(cinfo.image_height);
    img.channels = static_cast<int>(cinfo.num_components);

    if (!decompress) {
        jpeg_destroy_decompress(&cinfo);
        img.compressed = true;
        img.data = std::move(fileData);
        return img;
    }

    jpeg_start_decompress(&cinfo);

    int rowStride = cinfo.output_width * cinfo.output_components;
    img.channels = static_cast<int>(cinfo.output_components);
    img.data.resize(static_cast<size_t>(rowStride) * cinfo.output_height);

    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t* row = img.data.data() + cinfo.output_scanline * rowStride;
        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return img;
}

}  // namespace OpenSRX
