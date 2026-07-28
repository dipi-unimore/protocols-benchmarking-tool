#include "pbt/transport/TcpSender.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace pbt {

TcpSender::TcpSender(const BenchmarkConfig& cfg,
                     std::unique_ptr<PayloadSource> src,
                     std::unique_ptr<Serializer>    ser,
                     std::unique_ptr<Compressor>    cmp,
                     NtpInfo ntp_info)
    : Sender(cfg, std::move(src), std::move(ser), std::move(cmp), ntp_info) {}

TcpSender::~TcpSender() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

std::expected<void, Error> TcpSender::connect() {
    addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    auto port_str = std::to_string(config_.port);
    int rc = getaddrinfo(config_.host.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0)
        return std::unexpected(Error{std::format("getaddrinfo: {}", gai_strerror(rc))});
    fd_ = ::socket(res->ai_family, res->ai_socktype, 0);
    rc  = ::connect(fd_, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc != 0)
        return std::unexpected(Error{std::format("connect: {}", strerror(errno))});
    return {};
}

std::expected<void, Error> TcpSender::disconnect() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    return {};
}

std::expected<void, Error> TcpSender::do_send(std::span<const uint8_t> wire) {
    uint32_t len = static_cast<uint32_t>(wire.size());
    uint32_t len_ne = htonl(len);
    if (::send(fd_, &len_ne, 4, MSG_NOSIGNAL) != 4)
        return std::unexpected(Error{std::format("send length: {}", strerror(errno))});
    std::size_t sent = 0;
    while (sent < wire.size()) {
        ssize_t n = ::send(fd_, wire.data() + sent, wire.size() - sent, MSG_NOSIGNAL);
        if (n <= 0)
            return std::unexpected(Error{std::format("send data: {}", strerror(errno))});
        sent += static_cast<std::size_t>(n);
    }
    return {};
}

}  // namespace pbt
