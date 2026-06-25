#include "pbt/transport/TcpReceiver.hpp"
#include "pbt/payload/PayloadSource.hpp"
#include "pbt/serialization/Serializer.hpp"
#include "pbt/compression/Compressor.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace pbt {

TcpReceiver::TcpReceiver(const BenchmarkConfig& cfg,
                         BoundedBlockingQueue<InboundPacket>& queue,
                         int64_t ntp_offset_ns)
    : Receiver(cfg, queue, ntp_offset_ns) {}

TcpReceiver::~TcpReceiver() {
    if (server_fd_ >= 0) { ::close(server_fd_); server_fd_ = -1; }
}

std::expected<void, Error> TcpReceiver::bind() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        return std::unexpected(Error{std::format("socket: {}", strerror(errno))});
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(config_.port);
    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        return std::unexpected(Error{std::format("bind: {}", strerror(errno))});
    if (::listen(server_fd_, 5) != 0)
        return std::unexpected(Error{std::format("listen: {}", strerror(errno))});
    return {};
}

std::expected<void, Error> TcpReceiver::start() {
    accept_thread_ = std::jthread([this](std::stop_token st) {
        accept_loop(st);
    });
    return {};
}

std::expected<void, Error> TcpReceiver::stop() {
    if (server_fd_ >= 0) { ::close(server_fd_); server_fd_ = -1; }
    accept_thread_.request_stop();
    return {};
}

void TcpReceiver::accept_loop(std::stop_token st) {
    while (!st.stop_requested() && server_fd_ >= 0) {
        sockaddr_in client_addr{};
        socklen_t   addrlen = sizeof(client_addr);
        int client_fd = ::accept(server_fd_,
                                  reinterpret_cast<sockaddr*>(&client_addr),
                                  &addrlen);
        if (client_fd < 0) break;
        // Spawn a detached thread per client
        std::thread([this, client_fd, st]() {
            client_loop(client_fd, st);
        }).detach();
    }
}

void TcpReceiver::client_loop(int client_fd, std::stop_token st) {
    while (!st.stop_requested()) {
        uint32_t len_ne{};
        if (::recv(client_fd, &len_ne, 4, MSG_WAITALL) != 4) break;
        uint32_t len = ntohl(len_ne);
        Bytes buf(len);
        std::size_t got = 0;
        bool ok = true;
        while (got < len) {
            ssize_t n = ::recv(client_fd, buf.data() + got, len - got, 0);
            if (n <= 0) { ok = false; break; }
            got += static_cast<std::size_t>(n);
        }
        if (!ok) break;
        on_wire_bytes(buf);
    }
    ::close(client_fd);
}

}  // namespace pbt
