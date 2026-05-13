#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "OpenSRX/Image.hpp"
#include "OpenSRX/ImageServer.hpp"

namespace fs = std::filesystem;

namespace OpenSRX {

// ── BMP helpers ─────────────────────────────────────────────────────────────

/// Write a little-endian value into a buffer.
template <typename T>
static void writeLE(uint8_t* dst, T value) {
    for (size_t i = 0; i < sizeof(T); ++i) dst[i] = static_cast<uint8_t>((value >> (8 * i)) & 0xFF);
}

/// Generate a minimal valid BMP file in memory.
/// Pixels are written bottom-to-top (standard BMP row order).
static std::vector<uint8_t> makeBMP(int width, int height, int bitsPerPx,
                                    const std::vector<uint8_t>& pixelRows) {
    int rowBytes = width * (bitsPerPx / 8);
    int rowPadding = (4 - (rowBytes % 4)) % 4;
    int paddedRowBytes = rowBytes + rowPadding;

    uint32_t paletteSize = (bitsPerPx == 8) ? 256 * 4 : 0;
    uint32_t headerSize = 14 + 40 + paletteSize;
    uint32_t imageSize = paddedRowBytes * height;
    uint32_t fileSize = headerSize + imageSize;

    std::vector<uint8_t> bmp(fileSize, 0);

    // File header (14 bytes)
    bmp[0] = 'B';
    bmp[1] = 'M';
    writeLE<uint32_t>(bmp.data() + 2, fileSize);
    writeLE<uint32_t>(bmp.data() + 10, headerSize);

    // DIB header (BITMAPINFOHEADER, 40 bytes)
    writeLE<uint32_t>(bmp.data() + 14, 40);
    writeLE<int32_t>(bmp.data() + 18, width);
    writeLE<int32_t>(bmp.data() + 22, height);
    writeLE<uint16_t>(bmp.data() + 26, 1);  // planes
    writeLE<uint16_t>(bmp.data() + 28, bitsPerPx);
    writeLE<uint32_t>(bmp.data() + 30, 0);  // compression = BI_RGB

    // 8-bit grayscale palette
    if (bitsPerPx == 8) {
        for (int i = 0; i < 256; ++i) {
            uint32_t off = 54 + i * 4;
            bmp[off + 0] = static_cast<uint8_t>(i);  // B
            bmp[off + 1] = static_cast<uint8_t>(i);  // G
            bmp[off + 2] = static_cast<uint8_t>(i);  // R
            bmp[off + 3] = 0;                        // reserved
        }
    }

    // Pixel data (caller supplies rows bottom-to-top, no padding)
    for (int y = 0; y < height; ++y) {
        size_t srcOff = static_cast<size_t>(y) * rowBytes;
        size_t dstOff = headerSize + static_cast<size_t>(y) * paddedRowBytes;
        std::memcpy(bmp.data() + dstOff, pixelRows.data() + srcOff, rowBytes);
    }

    return bmp;
}

/// Write raw bytes to a file.
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
    // 2x2 image, BGR pixel data bottom-to-top:
    //   bottom row: blue(0,0,255), green(0,255,0)
    //   top row:    red(255,0,0), white(255,255,255)
    // BMP stores bottom row first.
    std::vector<uint8_t> pixels = {
        255, 0, 0,   0,   255, 0,    // bottom row: (B,G,R) = blue, green
        0,   0, 255, 255, 255, 255,  // top row: red, white
    };

    auto bmpData = makeBMP(2, 2, 24, pixels);
    std::string path = tmpDir + "/test24.bmp";
    writeFile(path, bmpData);

    Image img = decodeBMP(path);
    EXPECT_EQ(img.width, 2);
    EXPECT_EQ(img.height, 2);
    EXPECT_EQ(img.channels, 3);
    EXPECT_EQ(img.data.size(), 12u);

    // After decoding, row 0 = top of image (was the second row in BMP)
    // top row: red(0,0,255 in BGR), white(255,255,255)
    EXPECT_EQ(img.data[0], 0);
    EXPECT_EQ(img.data[1], 0);
    EXPECT_EQ(img.data[2], 255);
    EXPECT_EQ(img.data[3], 255);
    EXPECT_EQ(img.data[4], 255);
    EXPECT_EQ(img.data[5], 255);

    // bottom row: blue(255,0,0 in BGR), green(0,255,0)
    EXPECT_EQ(img.data[6], 255);
    EXPECT_EQ(img.data[7], 0);
    EXPECT_EQ(img.data[8], 0);
    EXPECT_EQ(img.data[9], 0);
    EXPECT_EQ(img.data[10], 255);
    EXPECT_EQ(img.data[11], 0);
}

TEST_F(TestBMPDecoder, Decode8Bit) {
    // 4x1 grayscale image
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
    // 1x1 BGRA pixel
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
    // 3x1 24-bit = 9 pixel bytes → padded to 12 bytes per row
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

// ── ImageServer tests ───────────────────────────────────────────────────────

TEST(TestImageServer, StartsAndStops) {
    ImageServer server(0);
    EXPECT_FALSE(server.isRunning());

    server.start();
    EXPECT_TRUE(server.isRunning());
    EXPECT_GT(server.getPort(), 0);
    EXPECT_FALSE(server.getRootPath().empty());
    EXPECT_TRUE(fs::exists(server.getRootPath()));

    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST(TestImageServer, DoubleStartIsNoop) {
    ImageServer server(0);
    server.start();
    uint16_t port1 = server.getPort();
    server.start();  // should not throw or restart
    EXPECT_EQ(server.getPort(), port1);
    server.stop();
}

TEST(TestImageServer, DestructorCleansUp) {
    std::string rootPath;
    {
        ImageServer server(0);
        server.start();
        rootPath = server.getRootPath();
        EXPECT_TRUE(fs::exists(rootPath));
    }
    // Destructor should remove the temp dir
    EXPECT_FALSE(fs::exists(rootPath));
}

TEST(TestImageServer, DetectsDroppedBMP) {
    ImageServer server(0);
    server.start();

    // Create a 2x2 24-bit BMP and drop it into the server root
    std::vector<uint8_t> pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    auto bmpData = makeBMP(2, 2, 24, pixels);
    writeFile(server.getRootPath() + "/image001.bmp", bmpData);

    // Wait for the watcher to pick it up (with timeout)
    Image img;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool got = false;
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.tryGetImage(img)) {
            got = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(got) << "Timed out waiting for image";
    EXPECT_EQ(img.width, 2);
    EXPECT_EQ(img.height, 2);
    EXPECT_EQ(img.channels, 3);

    server.stop();
}

TEST(TestImageServer, WaitForImageBlocks) {
    ImageServer server(0);
    server.start();

    // Drop a BMP after a short delay from another thread
    std::vector<uint8_t> pixels = {100, 200, 50};
    auto bmpData = makeBMP(1, 1, 24, pixels);

    std::thread writer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        writeFile(server.getRootPath() + "/delayed.bmp", bmpData);
    });

    Image img = server.waitForImage();
    writer.join();

    EXPECT_EQ(img.width, 1);
    EXPECT_EQ(img.height, 1);
    EXPECT_EQ(img.data[0], 100);
    EXPECT_EQ(img.data[1], 200);
    EXPECT_EQ(img.data[2], 50);

    server.stop();
}

TEST(TestImageServer, CallbackInvokedOnImage) {
    ImageServer server(0);

    bool callbackFired = false;
    int cbWidth = 0;
    server.setImageCallback([&](const Image& img) {
        callbackFired = true;
        cbWidth = img.width;
    });

    server.start();

    std::vector<uint8_t> pixels = {1, 2, 3, 4, 5, 6};
    auto bmpData = makeBMP(2, 1, 24, pixels);
    writeFile(server.getRootPath() + "/cb_test.bmp", bmpData);

    // Wait for callback
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!callbackFired && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(callbackFired);
    EXPECT_EQ(cbWidth, 2);

    server.stop();
}

TEST(TestImageServer, MultipleImagesReceived) {
    ImageServer server(0);
    server.start();

    // Drop two BMPs
    std::vector<uint8_t> px1 = {10, 20, 30};
    std::vector<uint8_t> px2 = {40, 50, 60};
    writeFile(server.getRootPath() + "/img1.bmp", makeBMP(1, 1, 24, px1));
    writeFile(server.getRootPath() + "/img2.bmp", makeBMP(1, 1, 24, px2));

    // Use waitForImage to reliably get both images
    Image img1 = server.waitForImage();
    Image img2 = server.waitForImage();

    EXPECT_EQ(img1.width, 1);
    EXPECT_EQ(img2.width, 1);

    server.stop();
}

TEST(TestImageServer, IgnoresNonBMPFiles) {
    ImageServer server(0);
    server.start();

    // Write a .txt file – should be ignored
    std::ofstream(server.getRootPath() + "/readme.txt") << "not an image";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Image img;
    EXPECT_FALSE(server.tryGetImage(img));

    server.stop();
}

TEST(TestImageServer, SubdirectoryImagesDetected) {
    ImageServer server(0);
    server.start();

    // Create a subfolder and drop a BMP in it
    std::string subdir = server.getRootPath() + "/subfolder";
    fs::create_directories(subdir);

    std::vector<uint8_t> pixels = {1, 2, 3};
    writeFile(subdir + "/nested.bmp", makeBMP(1, 1, 24, pixels));

    Image img;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool got = false;
    while (std::chrono::steady_clock::now() < deadline) {
        if (server.tryGetImage(img)) {
            got = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ASSERT_TRUE(got);
    EXPECT_EQ(img.width, 1);

    server.stop();
}

}  // namespace OpenSRX
