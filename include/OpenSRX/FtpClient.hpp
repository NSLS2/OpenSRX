#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace OpenSRX {

/**
 * @brief Minimal FTP client for downloading files from the scanner.
 *
 * The SR-X scanner runs an anonymous FTP server. This client connects,
 * logs in anonymously, and retrieves a single file in binary mode.
 */
class FtpClient {
   public:
    /**
     * @brief Download a file from an FTP server.
     *
     * Connects to the given host:port, logs in anonymously, and retrieves
     * the file at @p remotePath in binary (TYPE I) mode using PASV.
     *
     * @param host       FTP server hostname or IP.
     * @param port       FTP server port (usually 21).
     * @param remotePath Path to the file on the server (e.g. "/IMAGE/001_C_01.BMP").
     * @return The file contents as a byte vector.
     * @throws std::runtime_error on connection or transfer failure.
     */
    static std::vector<uint8_t> downloadFile(const std::string& host, uint16_t port,
                                             const std::string& remotePath);

    /**
     * @brief List file names in a directory on the FTP server.
     *
     * @param host       FTP server hostname or IP.
     * @param port       FTP server port (usually 21).
     * @param remotePath Directory path (e.g. "/IMAGE").
     * @return List of file/directory names in the directory.
     * @throws std::runtime_error on connection or listing failure.
     */
    static std::vector<std::string> listDirectory(const std::string& host, uint16_t port,
                                                  const std::string& remotePath);
};

}  // namespace OpenSRX
