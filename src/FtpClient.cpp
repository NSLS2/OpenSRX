#include "OpenSRX/FtpClient.hpp"

#include <spdlog/spdlog.h>

#include <asio.hpp>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace OpenSRX {

namespace {

// Read a single FTP response line (terminated by \r\n).
// Returns the full line including the status code.
std::string readResponse(asio::ip::tcp::socket& sock) {
    asio::streambuf buf;
    asio::read_until(sock, buf, "\r\n");
    std::istream is(&buf);
    std::string line;
    std::getline(is, line);
    // Remove trailing \r if present
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

// Read FTP response, handling multi-line responses (e.g. "220-...\r\n220 ...\r\n").
// Returns the final status line.
std::string readFtpResponse(asio::ip::tcp::socket& sock) {
    std::string result;
    asio::streambuf buf;

    while (true) {
        asio::read_until(sock, buf, "\r\n");
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        spdlog::debug("FTP << {}", line);
        result = line;

        // A final response line has format "NNN " (3 digits + space)
        if (line.size() >= 4 && std::isdigit(line[0]) && std::isdigit(line[1]) &&
            std::isdigit(line[2]) && line[3] == ' ') {
            break;
        }
    }
    return result;
}

// Send a command and return the response.
std::string sendCommand(asio::ip::tcp::socket& sock, const std::string& cmd) {
    std::string fullCmd = cmd + "\r\n";
    spdlog::debug("FTP >> {}", cmd);
    asio::write(sock, asio::buffer(fullCmd));
    return readFtpResponse(sock);
}

// Check that a response starts with the expected code.
void expectCode(const std::string& response, const std::string& code) {
    if (response.substr(0, code.size()) != code) {
        throw std::runtime_error("FTP error: expected " + code + ", got: " + response);
    }
}

// Parse PASV response to extract IP and port.
// Format: "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
std::pair<std::string, uint16_t> parsePasv(const std::string& response) {
    std::regex re(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    std::smatch match;
    if (!std::regex_search(response, match, re)) {
        throw std::runtime_error("Failed to parse PASV response: " + response);
    }
    std::string ip =
        match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
    uint16_t port =
        static_cast<uint16_t>(std::stoi(match[5].str()) * 256 + std::stoi(match[6].str()));
    return {ip, port};
}

}  // namespace

std::vector<uint8_t> FtpClient::downloadFile(const std::string& host, uint16_t port,
                                             const std::string& remotePath) {
    asio::io_context io;

    // Connect to FTP control port
    asio::ip::tcp::socket control(io);
    asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    asio::connect(control, endpoints);

    // Read welcome banner
    std::string resp = readFtpResponse(control);
    expectCode(resp, "220");

    // Login anonymously
    resp = sendCommand(control, "USER anonymous");
    // Accept 230 (logged in) or 331 (need password)
    if (resp.substr(0, 3) == "331") {
        resp = sendCommand(control, "PASS anonymous");
    }
    expectCode(resp, "230");

    // Binary mode
    resp = sendCommand(control, "TYPE I");
    expectCode(resp, "200");

    // Passive mode
    resp = sendCommand(control, "PASV");
    expectCode(resp, "227");
    auto [dataHost, dataPort] = parsePasv(resp);

    // Connect to data port
    asio::ip::tcp::socket data(io);
    auto dataEndpoints = resolver.resolve(dataHost, std::to_string(dataPort));
    asio::connect(data, dataEndpoints);

    // Request file
    resp = sendCommand(control, "RETR " + remotePath);
    // Accept 150 (opening data connection) or 125 (data connection already open)
    if (resp.substr(0, 3) != "150" && resp.substr(0, 3) != "125") {
        throw std::runtime_error("FTP RETR failed: " + resp);
    }

    // Read all data
    std::vector<uint8_t> fileData;
    asio::error_code ec;
    std::array<uint8_t, 8192> readBuf;
    while (true) {
        size_t n = data.read_some(asio::buffer(readBuf), ec);
        if (n > 0) {
            fileData.insert(fileData.end(), readBuf.begin(), readBuf.begin() + n);
        }
        if (ec == asio::error::eof) break;
        if (ec) throw std::runtime_error("FTP data read error: " + ec.message());
    }
    data.close();

    // Read transfer complete response
    resp = readFtpResponse(control);
    expectCode(resp, "226");

    // Quit
    sendCommand(control, "QUIT");
    control.close();

    spdlog::debug("FtpClient: downloaded {} ({} bytes)", remotePath, fileData.size());
    return fileData;
}

std::vector<std::string> FtpClient::listDirectory(const std::string& host, uint16_t port,
                                                  const std::string& remotePath) {
    asio::io_context io;

    // Connect to FTP control port
    asio::ip::tcp::socket control(io);
    asio::ip::tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, std::to_string(port));
    asio::connect(control, endpoints);

    // Read welcome banner
    std::string resp = readFtpResponse(control);
    expectCode(resp, "220");

    // Login anonymously
    resp = sendCommand(control, "USER anonymous");
    if (resp.substr(0, 3) == "331") {
        resp = sendCommand(control, "PASS anonymous");
    }
    expectCode(resp, "230");

    // ASCII mode for listing
    resp = sendCommand(control, "TYPE A");
    expectCode(resp, "200");

    // Passive mode
    resp = sendCommand(control, "PASV");
    expectCode(resp, "227");
    auto [dataHost, dataPort] = parsePasv(resp);

    // Connect to data port
    asio::ip::tcp::socket data(io);
    auto dataEndpoints = resolver.resolve(dataHost, std::to_string(dataPort));
    asio::connect(data, dataEndpoints);

    // Request name list
    resp = sendCommand(control, "NLST " + remotePath);
    if (resp.substr(0, 3) != "150" && resp.substr(0, 3) != "125") {
        throw std::runtime_error("FTP NLST failed: " + resp);
    }

    // Read listing data
    std::string listing;
    asio::error_code ec;
    std::array<char, 4096> readBuf;
    while (true) {
        size_t n = data.read_some(asio::buffer(readBuf), ec);
        if (n > 0) {
            listing.append(readBuf.data(), n);
        }
        if (ec == asio::error::eof) break;
        if (ec) throw std::runtime_error("FTP data read error: " + ec.message());
    }
    data.close();

    // Read transfer complete response
    resp = readFtpResponse(control);
    expectCode(resp, "226");

    // Quit
    sendCommand(control, "QUIT");
    control.close();

    // Parse listing into filenames (one per line)
    std::vector<std::string> files;
    std::istringstream iss(listing);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) {
            // NLST may return full paths; extract just the filename
            auto pos = line.rfind('/');
            if (pos != std::string::npos) {
                line = line.substr(pos + 1);
            }
            if (!line.empty()) {
                files.push_back(line);
            }
        }
    }

    spdlog::debug("FtpClient: listed {} ({} entries)", remotePath, files.size());
    return files;
}

}  // namespace OpenSRX
