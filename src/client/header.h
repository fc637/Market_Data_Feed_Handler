// include/header.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "protocol.h" // for MemoryPool and LatencyTracker from common folder

class MarketDataSocket {
public:
    MarketDataSocket();
    ~MarketDataSocket();

    bool connect(const std::string& host, uint16_t port,
                 uint32_t timeout_ms = 5000);

    ssize_t receive(void* buffer, size_t max_len);

    bool send_subscription(const std::vector<uint16_t>& symbol_ids);

    bool is_connected() const;
    void disconnect();

    bool set_tcp_nodelay(bool enable);
    bool set_recv_buffer_size(size_t bytes);
    bool set_socket_priority(int priority);

    int epoll_fd() const { return epoll_fd_; }
    int socket_fd() const { return sock_fd_; }

private:
    bool wait_for_connect(uint32_t timeout_ms);
    void set_nonblocking(int fd);

private:
    int sock_fd_{-1};
    int epoll_fd_{-1};
    bool connected_{false};

    MemoryPool recv_pool_{64 * 1024, 64};   // 64KB buffers
    // LatencyTracker latency_;
};

struct Tick;

class MarketDataParser {
public:
    using TickCallback = std::function<void(const Tick&)>;

    explicit MarketDataParser(size_t num_symbols); //MarketDataParser();

    // Consume raw TCP bytes
    void consume(const uint8_t* data,
                 size_t len,
                 TickCallback on_tick);

private:
    static constexpr size_t MAX_BUFFER = 1 << 20;
    static constexpr size_t HEADER_SIZE = 16;
    static constexpr size_t CHECKSUM_SIZE = 4;

    alignas(64) uint8_t buffer_[MAX_BUFFER];
    size_t write_pos_;
    size_t read_pos_;
    // uint32_t last_seq_;
    std::vector<uint32_t> last_seq_per_symbol_;

    void reset();
    void parse_loop(TickCallback on_tick);
    void drop_bytes(size_t n);
    void compact();
};

class Visualizer {
public:
    Visualizer(const LockFreeSymbolCache& cache,
               size_t num_symbols)
        : cache_(cache),
          num_symbols_(num_symbols),
          running_(true) {
        setup_stdin();
        start_time_ = std::chrono::steady_clock::now();
    }

    void run();
    void stop() { running_ = false; }

private:
    void setup_stdin();
    // void restore_stdin();
    void render();
    void handle_input();
    void clear_screen();
    void print_header(uint64_t msg_count);
    void print_table(const std::vector<size_t>& top);
    void restore_stdin();
    const LockFreeSymbolCache& cache_;
    size_t num_symbols_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point start_time_;
};

