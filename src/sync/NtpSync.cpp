#include "pbt/sync/NtpSync.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <format>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace pbt {

// RFC 4330 SNTP packet (48 bytes)
struct SntpPacket {
    uint8_t  li_vn_mode{0x1Bu};    // LI=0, VN=3, Mode=3 (client)
    uint8_t  stratum{0};
    uint8_t  poll{0};
    uint8_t  precision{0};
    uint32_t root_delay{0};
    uint32_t root_dispersion{0};
    uint32_t ref_id{0};
    uint32_t ref_ts_sec{0};
    uint32_t ref_ts_frac{0};
    uint32_t orig_ts_sec{0};
    uint32_t orig_ts_frac{0};
    uint32_t recv_ts_sec{0};
    uint32_t recv_ts_frac{0};
    uint32_t tx_ts_sec{0};
    uint32_t tx_ts_frac{0};
};
static_assert(sizeof(SntpPacket) == 48);

// NTP epoch offset: seconds from Jan 1 1900 to Jan 1 1970
static constexpr uint32_t kNtpEpochOffset = 2208988800u;

static int64_t ntp_to_ns(uint32_t sec_be, uint32_t frac_be) {
    uint32_t sec  = ntohl(sec_be)  - kNtpEpochOffset;
    uint32_t frac = ntohl(frac_be);
    int64_t ns = static_cast<int64_t>(sec) * 1'000'000'000LL
               + (static_cast<int64_t>(frac) * 1'000'000'000LL >> 32);
    return ns;
}

std::expected<int64_t, Error>
NtpSync::query(const std::string& server, int retries,
               std::chrono::milliseconds timeout) {
    addrinfo hints{}, *res{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(server.c_str(), "123", &hints, &res) != 0)
        return std::unexpected(Error{std::format("NTP: getaddrinfo failed for {}", server)});

    for (int attempt = 0; attempt < retries; ++attempt) {
        int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) continue;

        // Recv timeout
        timeval tv{};
        tv.tv_sec  = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        SntpPacket req{};
        req.li_vn_mode = 0x1Bu;

        int64_t t1 = now_ns();
        if (::sendto(fd, &req, sizeof(req), 0, res->ai_addr, res->ai_addrlen) < 0) {
            ::close(fd); continue;
        }

        SntpPacket rsp{};
        ssize_t n = ::recv(fd, &rsp, sizeof(rsp), 0);
        int64_t t4 = now_ns();
        ::close(fd);
        if (n < static_cast<ssize_t>(sizeof(rsp))) continue;

        int64_t t2 = ntp_to_ns(rsp.recv_ts_sec, rsp.recv_ts_frac);
        int64_t t3 = ntp_to_ns(rsp.tx_ts_sec,   rsp.tx_ts_frac);

        // Offset = ((T2 - T1) + (T3 - T4)) / 2
        int64_t offset_ns = ((t2 - t1) + (t3 - t4)) / 2;
        freeaddrinfo(res);
        return offset_ns;
    }
    freeaddrinfo(res);
    return std::unexpected(Error{std::format(
        "NTP: all {} retries failed for {}", retries, server)});
}

}  // namespace pbt
