#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "protocol.h" 
#include "header.h" 
// #include "../common/memory_pool.h"
// #include "../common/latency_tracker.h"

using namespace std;

/* ---------------- MarketDataSocket ---------------- */


/* ---------------- Implementation ---------------- */

MarketDataSocket::MarketDataSocket() {
    epoll_fd_ = epoll_create1(0);
}

MarketDataSocket::~MarketDataSocket() {
    disconnect();
    if (epoll_fd_ >= 0) close(epoll_fd_);
}

void MarketDataSocket::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool MarketDataSocket::connect(const std::string& host,
                               uint16_t port,
                               uint32_t timeout_ms) {
    sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd_ < 0) return false;

    set_nonblocking(sock_fd_);
    set_tcp_nodelay(true);
    set_recv_buffer_size(4 * 1024 * 1024);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    int rc = ::connect(sock_fd_, (sockaddr*)&addr, sizeof(addr));
    if (rc < 0 && errno != EINPROGRESS) {
        close(sock_fd_);
        return false;
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sock_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_fd_, &ev);

    connected_ = wait_for_connect(timeout_ms);
    return connected_;
}

bool MarketDataSocket::wait_for_connect(uint32_t timeout_ms) {
    epoll_event ev;
    int n = epoll_wait(epoll_fd_, &ev, 1, timeout_ms);
    if (n <= 0) return false;

    int err;
    socklen_t len = sizeof(err);
    getsockopt(sock_fd_, SOL_SOCKET, SO_ERROR, &err, &len);
    return err == 0;
}

ssize_t MarketDataSocket::receive(void* buffer, size_t max_len) {
    auto before = std::chrono::high_resolution_clock::now();

    // ssize_t n = ::recv(fd_, buffer, max_len, MSG_DONTWAIT);
    ssize_t n = ::recv(sock_fd_, buffer, max_len, MSG_DONTWAIT);

    if (n > 0) {
        auto after = std::chrono::high_resolution_clock::now();
        uint64_t latency_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count();

        LatencyTracker::instance().record_kernel_to_user(latency_ns);
        return n;
    }

    if (n == 0) {
        // FIN received
        connected_ = false;
        return 0;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return -1; // no data now
    }

    connected_ = false;
    return -1;
}


// ssize_t MarketDataSocket::receive(void* buffer, size_t max_len) {
//     ssize_t n = recv(sock_fd_, buffer, max_len, 0);

//     if (n > 0) {
//         uint64_t ts =
//             chrono::duration_cast<chrono::nanoseconds>(
//                 chrono::high_resolution_clock::now().time_since_epoch()).count();
//         latency_.record_kernel_to_user(ts);
//         return n;
//     }

//     if (n == 0) {
//         connected_ = false;   // FIN
//         return 0;
//     }

//     if (errno == EAGAIN || errno == EWOULDBLOCK)
//         return -1;

//     connected_ = false;
//     return -1;
// }

bool MarketDataSocket::send_subscription(
        const std::vector<uint16_t>& symbol_ids) {

    uint16_t count = symbol_ids.size();
    size_t len = 1 + 2 + count * 2;

    std::vector<uint8_t> buf(len);
    buf[0] = 0xFF;
    memcpy(&buf[1], &count, 2);
    memcpy(&buf[3], symbol_ids.data(), count * 2);

    return send(sock_fd_, buf.data(), buf.size(), 0) == (ssize_t)buf.size();
}

bool MarketDataSocket::set_tcp_nodelay(bool enable) {
    int val = enable ? 1 : 0;
    return setsockopt(sock_fd_, IPPROTO_TCP,
                      TCP_NODELAY, &val, sizeof(val)) == 0;
}

bool MarketDataSocket::set_recv_buffer_size(size_t bytes) {
    return setsockopt(sock_fd_, SOL_SOCKET,
                      SO_RCVBUF, &bytes, sizeof(bytes)) == 0;
}

bool MarketDataSocket::set_socket_priority(int priority) {
    return setsockopt(sock_fd_, SOL_SOCKET,
                      SO_PRIORITY, &priority, sizeof(priority)) == 0;
}

bool MarketDataSocket::is_connected() const {
    return connected_;
}

void MarketDataSocket::disconnect() {
    if (sock_fd_ >= 0) {
        close(sock_fd_);
        sock_fd_ = -1;
    }
    connected_ = false;
}
