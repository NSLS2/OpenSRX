#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "OpenSRX/Image.hpp"

namespace fs = std::filesystem;

namespace OpenSRX {

// ── BMP helpers ─────────────────────────────────────────────────────────────

template <typename T>
static void writeLE(uint8_t* dst, T value) {
    for (size_t i = 0; i < sizeof(T); ++i) dst[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
}

/// Generate a minimal valid BMP file in memory.
static std::vector<uint8_t> makeBMP(int width, int height, int bitsPerPx,
                                    const std::vector<uint8_t>& pixelRows) {
    int rowBytes = width * (bitsPerPx / 8);
    int rowPadding = (4 - (rowBytes % 4)) % 4;
    int paddedRowBytes = rowBytes + rowPadding;

    int paletteSize = (bitsPerPx == 8) ? 256 * 4 : 0;
    int pixelDataOffset = 14 + 40 + paletteSize;
    int imageSize = paddedRowBytes * height;
    int fileSize = pixelDataOffset + imageSize;

    std::vector<uint8_t> bmp(fileSize, 0);
    uint8_t* p = bmp.data();

    // File header (14 bytes)
    p[0] = 'B'; p[1] = 'M';
    writeLE<uint32_t>(p + 2, fileSize);
    writeLE<uint32_t>(p + 10, pixelDataOffset);

    // DIB header (40 bytes)
    writeLE<uint32_t>(p + 14, 40);
    writeLE<int32_t>(p + 18, width);
    writeLE<int32_t>(p + 22, height);
    writeLE<uint16_t>(p + 26, 1);
    writeLE<uint16_t>(p + 28, bitsPerPx);
    writeLE<uint32_t>(p + 34, imageSize);

    // Palette for 8-bit
    if (bitsPerPx == 8) {
        uint8_t* palette = p + 54;
        for (int i = 0; i < 256; ++i) {
            palette[i * 4 + 0] = static_cast<uint8_t>(i);
            palette[i * 4 + 1] = static_cast<uint8_t>(i);
            palette[i * 4 + 2] = static_cast<uint8_t>(i);
            palette[i * 4 + 3] = 0;
        }
    }

    // Pixel data (caller provides rows bottom-to-top, unpadded)
    uint8_t* pixelDst = p + pixelDataOffset;
    for (int y = 0; y < height; ++y) {
        const uint8_t* src = pixelRows.data() + y * rowBytes;
        std::memcpy(pixelDst + y * paddedRowBytes, src, rowBytes);
    }

    return bmp;
}

static void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

// ── decodeBMP tests ─────────────────────────────────────────────────────────

class TestBMPDecoder : public ::testing::Test {
   protected:
    std::string tmpDir;

    void SetUp() override {
        std::string tmpl = (fs::temp_directory_path() / "opensrx-test-XXXXXX").string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        char* result = mkdtemp(buf.data());
        ASSERT_NE(result, nullptr);
        tmpDir = std::string(result);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tmpDir, ec);
    }
};

TEST_F(TestBMPDecoder, Decode24Bit) {
    std::vector<uint8_t> pixels = {
        255, 0, 0,   0,   255, 0,
        0,   0, 255, 255, 255, 255,
    };

    auto bmpData = makeBMP(2, 2, 24, pixels);
    std::string path = tmpDir + "/test24.bmp";
    writeFile(path, bmpData);

    Image img = decodeBMP(path);
    EXPECT_EQ(img.width, 2);
    EXPECT_EQ(img.height, 2);
    EXPECT_EQ(img.channels, 3);
    EXPECT_EQ(img.data.size(), 12u);

    // After decoding, row 0 = top (was second row in BMP)
    EXPECT_EQ(img.data[0], 0);
    EXPECT_EQ(img.data[1], 0);
    EXPECT_EQ(img.data[2], 255);
    EXPECT_EQ(img.data[3], 255);
    EXPECT_EQ(img.data[4], 255);
    EXPECT_EQ(img.data[5], 255);

    EXPECT_EQ(img.data[6], 255);
    EXPECT_EQ(img.data[7], 0);
    EXPECT_EQ(img.data[8], 0);
    EXPECT_EQ(img.data[9], 0);
    EXPECT_EQ(img.data[10], 255);
    EXPECT_EQ(img.data[11], 0);
}

TEST_F(TestBMPDecoder, Decode8Bit) {
    std::vector<uint8_t> pixels = {0, 64, 128, 255};
    auto bmpData = makeBMP(4, 1, 8, pixels);
    std::string path = tmpDir + "/test8.bmp";
    writeFile(path, bmpData);

    Image img = decodeBMP(path);
    EXPECT_EQ(img.width, 4);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.channels, 1);
    ASSERT_EQ(img.data.size(), 4u);
    EXPECT_EQ(img.data[0], 0);
    EXPECT_EQ(img.data[1], 64);
    EXPECT_EQ(img.data[2], 128);
    EXPECT_EQ(img.data[3], 255);
}

TEST_F(TestBMPDecoder, Decode32Bit) {
    std::vector<uint8_t> pixels = {10, 20, 30, 40};
    auto bmpData = makeBMP(1, 1, 32, pixels);
    std::string path = tmpDir + "/test32.bmp";
    writeFile(path, bmpData);

    Image img = decodeBMP(path);
    EXPECT_EQ(img.width, 1);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.channels, 4);
    ASSERT_EQ(img.data.size(), 4u);
    EXPECT_EQ(img.data[0], 10);
    EXPECT_EQ(img.data[1], 20);
    EXPECT_EQ(img.data[2], 30);
    EXPECT_EQ(img.data[3], 40);
}

TEST_F(TestBMPDecoder, RowPaddingHandled) {
    std::vector<uint8_t> pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto bmpData = makeBMP(3, 1, 24, pixels);
    std::string path = tmpDir + "/testpad.bmp";
    writeFile(path, bmpData);

    Image img = decodeBMP(path);
    EXPECT_EQ(img.width, 3);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.channels, 3);
    ASSERT_EQ(img.data.size(), 9u);
    EXPECT_EQ(img.data[0], 1);
    EXPECT_EQ(img.data[8], 9);
}

TEST_F(TestBMPDecoder, InvalidFileThrows) {
    EXPECT_THROW(decodeBMP(tmpDir + "/nonexistent.bmp"), std::runtime_error);

    std::string path = tmpDir + "/notbmp.bmp";
    writeFile(path, {0, 0, 0, 0});
    EXPECT_THROW(decodeBMP(path), std::runtime_error);
}

// ── decodeBMPFromMemory tests ───────────────────────────────────────────────

TEST(TestBMPFromMemory, Decode24Bit) {
    std::vector<uint8_t> pixels = {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255};
    auto bmpData = makeBMP(2, 2, 24, pixels);

    Image img = decodeBMPFromMemory(bmpData);
    EXPECT_EQ(img.width, 2);
    EXPECT_EQ(img.height, 2);
    EXPECT_EQ(img.channels, 3);
    EXPECT_EQ(img.data.size(), 12u);
}

TEST(TestBMPFromMemory, Decode8Bit) {
    std::vector<uint8_t> pixels = {10, 20, 30, 40};
    auto bmpData = makeBMP(4, 1, 8, pixels);

    Image img = decodeBMPFromMemory(bmpData);
    EXPECT_EQ(img.width, 4);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.channels, 1);
    ASSERT_EQ(img.data.size(), 4u);
    EXPECT_EQ(img.data[0], 10);
    EXPECT_EQ(img.data[3], 40);
}

TEST(TestBMPFromMemory, TruncatedDataThrows) {
    std::vector<uint8_t> pixels = {1, 2, 3};
    auto bmpData = makeBMP(1, 1, 24, pixels);
    bmpData.resize(bmpData.size() - 10);  // truncate
    EXPECT_THROW(decodeBMPFromMemory(bmpData), std::runtime_error);
}

TEST(TestBMPFromMemory, InvalidHeaderThrows) {
    std::vector<uint8_t> bad = {0, 0, 0, 0};
    EXPECT_THROW(decodeBMPFromMemory(bad), std::runtime_error);
}

}  // namespace OpenSRX
