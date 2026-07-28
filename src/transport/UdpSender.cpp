#include "pbt/transport/UdpSender.hpp"
#include "pbt/core/FragmentHeader.hpp"
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

UdpSender::UdpSender(const BenchmarkConfig& cfg,
                     std::unique_ptr<PayloadSource> src,
                     std::unique_ptr<Serializer>    ser,
                     std::unique_ptr<Compressor>    cmp,
                     NtpInfo ntp_info)
    : Sender(cfg, std::move(src), std::move(ser), std::move(cmp), ntp_info) {}

UdpSender::~UdpSender() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

std::expected<void, Error> UdpSender::connect() {
    addrinfo hints{}, *res;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    auto port_str = std::to_string(config_.port);
    int rc = getaddrinfo(config_.host.c_str(), port_str.c_str(), &hints, &res);
    if (rc != 0)
        return std::unexpected(Error{std::format("getaddrinfo: {}", gai_strerror(rc))});
    fd_ = ::socket(res->ai_family, res->ai_socktype, 0);
    rc  = ::connect(fd_, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc != 0)
        return std::unexpected(Error{std::format("UDP connect: {}", strerror(errno))});
    return {};
}

std::expected<void, Error> UdpSender::disconnect() {
    if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    return {};
}

std::expected<void, Error> UdpSender::send_datagram(std::span<const uint8_t> data) {
    ssize_t n = ::send(fd_, data.data(), data.size(), 0);
    if (n < 0)
        return std::unexpected(Error{std::format("UDP sendto: {}", strerror(errno))});
    return {};
}

std::expected<void, Error> UdpSender::do_send(std::span<const uint8_t> wire) {
    // Fragment if wire > MTU
    const uint32_t mtu  = config_.udp_mtu;
    const uint32_t fhdr_size = static_cast<uint32_t>(sizeof(FragmentHeader));
    const uint32_t max_data  = mtu - fhdr_size;

    if (wire.size() <= mtu) {
        // Single datagram (no fragmentation) — still prepend FragmentHeader
        FragmentHeader fhdr;
        // Extract message_id from WireHeader at start of wire
        WireHeader wh;
        std::memcpy(&wh, wire.data(), sizeof(WireHeader));
        fhdr.message_id     = wh.sequence_id;
        fhdr.frag_index     = 0;
        fhdr.frag_count     = 1;
        fhdr.frag_data_size = static_cast<uint32_t>(wire.size());
        Bytes dgram(fhdr_size + wire.size());
        std::memcpy(dgram.data(), &fhdr, fhdr_size);
        std::memcpy(dgram.data() + fhdr_size, wire.data(), wire.size());
        return send_datagram(dgram);
    }

    WireHeader wh;
    std::memcpy(&wh, wire.data(), sizeof(WireHeader));
    const uint64_t msg_id    = wh.sequence_id;
    const uint32_t total     = static_cast<uint32_t>((wire.size() + max_data - 1) / max_data);
    uint32_t offset = 0;
    for (uint32_t i = 0; i < total; ++i) {
        uint32_t chunk = std::min(max_data, static_cast<uint32_t>(wire.size()) - offset);
        FragmentHeader fhdr;
        fhdr.message_id     = msg_id;
        fhdr.frag_index     = static_cast<uint16_t>(i);
        fhdr.frag_count     = static_cast<uint16_t>(total);
        fhdr.frag_data_size = chunk;
        Bytes dgram(fhdr_size + chunk);
        std::memcpy(dgram.data(), &fhdr, fhdr_size);
        std::memcpy(dgram.data() + fhdr_size, wire.data() + offset, chunk);
        auto res = send_datagram(dgram);
        if (!res) return res;
        offset += chunk;
    }
    return {};
}

}  // namespace pbt
