#pragma once

#include <asio.hpp>
#include <string>
#include <utility>

namespace OpenSRX {
namespace detail {

/**
 * @brief Internal wire-transport interface.
 *
 * Provides raw byte-level write/read operations used by ICommInterface
 * to implement command framing and parsing.
 */
class IWireTransport {
   public:
    virtual ~IWireTransport() = default;

    virtual void write(const std::string& data) = 0;
    virtual std::string readUntil(const std::string& terminator) = 0;
};

/**
 * @brief Internal Asio-based wire-transport base template.
 */
template <typename StreamT>
class AsioWireTransport : public IWireTransport {
   public:
    ~AsioWireTransport() override = default;

    void write(const std::string& data) override { asio::write(stream, asio::buffer(data)); }

    std::string readUntil(const std::string& terminator) override {
        asio::streambuf response;
        asio::read_until(stream, response, terminator);
        return {asio::buffers_begin(response.data()), asio::buffers_end(response.data())};
    }

   protected:
    asio::io_context ioContext;
    StreamT stream;

    template <typename... Args>
    explicit AsioWireTransport(Args&&... args) : stream(std::forward<Args>(args)...) {}
};

}  // namespace detail
}  // namespace OpenSRX
