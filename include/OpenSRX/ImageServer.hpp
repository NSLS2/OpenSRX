#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <fineftp/server.h>

#include "OpenSRX/Image.hpp"

namespace OpenSRX {

/**
 * @brief Embedded FTP server for receiving images from the scanner.
 *
 * Creates a temporary directory (under /tmp) and starts a fineftp FTP server.
 * A background thread polls the directory for new BMP files, decodes them,
 * and stores them in a queue that can be consumed by the caller.
 */
class ImageServer {
   public:
    /**
     * @brief Construct (but do not start) the image server.
     *
     * @param port       FTP port to listen on (0 = OS picks a free port).
     * @param username   FTP username the scanner will log in with.
     * @param password   FTP password the scanner will log in with.
     */
    ImageServer(uint16_t port = 0,
                const std::string& username = "opensrx",
                const std::string& password = "opensrx");

    ~ImageServer();

    // Non-copyable / non-movable
    ImageServer(const ImageServer&) = delete;
    ImageServer& operator=(const ImageServer&) = delete;

    /** Start the FTP server and the background watcher thread. */
    void start();

    /** Stop the FTP server and watcher thread. */
    void stop();

    /** @return true if the server is running. */
    bool isRunning() const { return running.load(); }

    /** @return The port the FTP server is listening on (valid after start()). */
    uint16_t getPort() const;

    /** @return The FTP username. */
    std::string getUsername() const { return username; }

    /** @return The FTP password. */
    std::string getPassword() const { return password; }

    /** @return The local filesystem path where images are received. */
    std::string getRootPath() const { return rootPath; }

    /**
     * @brief Block until an image is available, then return it.
     *
     * @return The next decoded Image, or a default Image if the server is
     *         stopped while waiting.
     */
    Image waitForImage();

    /**
     * @brief Return the next image if one is available, without blocking.
     *
     * @param[out] image  Filled with the image data on success.
     * @return true if an image was available.
     */
    bool tryGetImage(Image& image);

    /**
     * @brief Return all images currently queued.
     */
    std::deque<Image> getImages();

    /**
     * @brief Set a callback invoked each time a new image is decoded.
     *
     * The callback runs on the watcher thread; keep it fast.
     */
    void setImageCallback(std::function<void(const Image&)> cb);

   private:
    void watchDirectory();

    uint16_t port;
    std::string username;
    std::string password;
    std::string rootPath;

    std::unique_ptr<fineftp::FtpServer> ftpServer;

    std::atomic<bool> running{false};
    std::thread watcherThread;

    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::deque<Image> imageQueue;

    std::function<void(const Image&)> imageCallback;
};

}  // namespace OpenSRX
