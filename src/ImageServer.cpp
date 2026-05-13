#include "OpenSRX/ImageServer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <set>

#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace OpenSRX {

ImageServer::ImageServer(uint16_t port, const std::string& username,
                         const std::string& password)
    : port(port), username(username), password(password) {
    // Create a temporary directory for FTP file storage
    std::string tmpl = (fs::temp_directory_path() / "opensrx-ftp-XXXXXX").string();
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');

    char* result = mkdtemp(buf.data());
    if (!result)
        throw std::runtime_error("Failed to create temporary directory for ImageServer");

    rootPath = std::string(result);
    spdlog::info("ImageServer: created temp directory {}", rootPath);
}

ImageServer::~ImageServer() {
    stop();

    // Clean up temporary directory
    std::error_code ec;
    fs::remove_all(rootPath, ec);
    if (ec) spdlog::warn("ImageServer: failed to remove {}: {}", rootPath, ec.message());
}

void ImageServer::start() {
    if (running.load()) return;

    ftpServer = std::make_unique<fineftp::FtpServer>(port);
    ftpServer->addUser(username, password, rootPath,
                       fineftp::Permission::FileWrite | fineftp::Permission::DirCreate |
                           fineftp::Permission::DirList);
    ftpServer->start(1);

    spdlog::info("ImageServer: FTP server started on port {} (root: {})", ftpServer->getPort(),
                 rootPath);

    running.store(true);
    watcherThread = std::thread(&ImageServer::watchDirectory, this);
}

void ImageServer::stop() {
    if (!running.load()) return;

    running.store(false);
    queueCV.notify_all();

    if (watcherThread.joinable()) watcherThread.join();

    if (ftpServer) {
        ftpServer->stop();
        ftpServer.reset();
    }

    spdlog::info("ImageServer: stopped");
}

uint16_t ImageServer::getPort() const {
    if (ftpServer) return ftpServer->getPort();
    return port;
}

Image ImageServer::waitForImage() {
    std::unique_lock lock(queueMutex);
    queueCV.wait(lock, [this] { return !imageQueue.empty() || !running.load(); });
    if (imageQueue.empty()) return {};
    Image img = std::move(imageQueue.front());
    imageQueue.pop_front();
    return img;
}

bool ImageServer::tryGetImage(Image& image) {
    std::lock_guard lock(queueMutex);
    if (imageQueue.empty()) return false;
    image = std::move(imageQueue.front());
    imageQueue.pop_front();
    return true;
}

std::deque<Image> ImageServer::getImages() {
    std::lock_guard lock(queueMutex);
    std::deque<Image> result;
    result.swap(imageQueue);
    return result;
}

void ImageServer::setImageCallback(std::function<void(const Image&)> cb) {
    imageCallback = std::move(cb);
}

void ImageServer::watchDirectory() {
    std::set<std::string> processed;

    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::error_code ec;
        for (auto it = fs::recursive_directory_iterator(rootPath, ec);
             it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;

            if (!it->is_regular_file()) continue;

            const auto& path = it->path();
            std::string pathStr = path.string();

            // Already decoded this file
            if (processed.count(pathStr)) continue;

            // Only handle .bmp files
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".bmp") continue;

            // Check that the file isn't still being written (size must be stable)
            auto size1 = fs::file_size(path, ec);
            if (ec) continue;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto size2 = fs::file_size(path, ec);
            if (ec || size1 != size2 || size1 == 0) continue;

            try {
                Image img = decodeBMP(pathStr);
                spdlog::debug("ImageServer: decoded {} ({}x{}, {} ch)", pathStr, img.width,
                              img.height, img.channels);

                processed.insert(pathStr);

                if (imageCallback) imageCallback(img);

                {
                    std::lock_guard lock(queueMutex);
                    imageQueue.push_back(std::move(img));
                }
                queueCV.notify_one();

            } catch (const std::exception& e) {
                spdlog::warn("ImageServer: failed to decode {}: {}", pathStr, e.what());
            }
        }
    }
}

}  // namespace OpenSRX
