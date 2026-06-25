#include "pbt/transport/UdpReceiver.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace pbt {

UdpReceiver::UdpReceiver(const BenchmarkConfig& cfg,
                         BoundedBlockingQueue<InboundPacket>& queue,
                         int64_t ntp_offset_ns)
    : Receiver(cfg, queue, ntp_offset_ns) {}

UdpReceiver::~UdpReceiver() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

std::expected<void, Error> UdpReceiver::bind() {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
        return std::unexpected(Error{std::format("socket: {}", strerror(errno))});
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(config_.port);
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
        return std::unexpected(Error{std::format("bind: {}", strerror(errno))});
    return {};
}

std::expected<void, Error> UdpReceiver::start() {
    thread_ = std::jthread([this](std::stop_token st) { recv_loop(st); });
    return {};
}

std::expected<void, Error> UdpReceiver::stop() {
    thread_.request_stop();
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    return {};
}

void UdpReceiver::recv_loop(std::stop_token st) {
    constexpr std::size_t kBuf = 65536;
    Bytes buf(kBuf);
    while (!st.stop_requested() && fd_ >= 0) {
        ssize_t n = ::recv(fd_, buf.data(), buf.size(), 0);
        if (n <= 0) break;
        auto data = std::span<const uint8_t>(buf.data(), static_cast<std::size_t>(n));
        if (data.size() < sizeof(FragmentHeader)) continue;
        FragmentHeader fhdr;
        std::memcpy(&fhdr, data.data(), sizeof(FragmentHeader));
        if (fhdr.magic != 0x50425446u) continue;
        auto frag_data = data.subspan(sizeof(FragmentHeader));
        reassembly_.insert(fhdr, frag_data, [this](Bytes wire) {
            on_wire_bytes(wire);
        });
    }
}

}  // namespace pbt
