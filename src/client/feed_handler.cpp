// src/client/feed_handler.cpp
#include <sys/epoll.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <cstring>
#include "header.h"
#include "protocol.h"        // Tick, SymbolCache
// socket.cpp and parser.cpp expose their classes internally

// Forward declarations (no headers by design)
// class MarketDataSocket;
// class Parser;

class FeedHandler {
public:
    FeedHandler(const std::string& host,
                uint16_t port,
                LockFreeSymbolCache& cache)
        : host_(host),
          port_(port),
          cache_(cache),
          epoll_fd_(-1),
          running_(true) {}

    void run();

private:
    bool connect_with_retry();
    void setup_epoll();
    void shutdown();

private:
    std::string host_;
    uint16_t port_;

    LockFreeSymbolCache& cache_; 

    int epoll_fd_;
    bool running_;

    MarketDataSocket socket_;
    MarketDataParser parser_;
};

bool FeedHandler::connect_with_retry() {
    constexpr int MAX_RETRIES = 5;
    int backoff_ms = 100;

    for (int attempt = 0; attempt < MAX_RETRIES && running_; ++attempt) {
        std::cout << "[feed] Connecting to "
                  << host_ << ":" << port_ << "\n";

        if (socket_.connect(host_, port_)) {
            socket_.set_tcp_nodelay(true);
            socket_.set_recv_buffer_size(4 * 1024 * 1024);
            return true;
        }

        std::cout << "[feed] Connect failed, retrying in "
                  << backoff_ms << " ms\n";

        std::this_thread::sleep_for(
            std::chrono::milliseconds(backoff_ms));

        backoff_ms *= 2; // exponential backoff
    }

    return false;
}

void FeedHandler::setup_epoll() {
    epoll_fd_ = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    // ev.data.fd = socket_.fd(); // socket exposes fd()
    ev.data.fd = socket_.socket_fd();

    // epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_.fd(), &ev);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_.socket_fd(), &ev);
}

void FeedHandler::run() {
    if (!connect_with_retry()) {
        std::cerr << "[feed] Unable to connect, exiting\n";
        return;
    }

    setup_epoll();

    constexpr size_t RX_BUF_SIZE = 64 * 1024;
    std::vector<char> rx_buffer(RX_BUF_SIZE);

    epoll_event events[8];

    while (running_) {
        int n = epoll_wait(epoll_fd_, events, 8, 1000);

        if (n < 0)
            continue;

        for (int i = 0; i < n; ++i) {
            if (!(events[i].events & EPOLLIN))
                continue;

            // Read until EAGAIN (edge-triggered requirement)
            while (true) {
                ssize_t bytes =
                    socket_.receive(rx_buffer.data(), rx_buffer.size());

                if (bytes > 0) {

                    parser_.consume(reinterpret_cast<const uint8_t*>(rx_buffer.data()),bytes,[&](const Tick& tick) {
                        // Trade update
                        if (tick.trade_qty > 0) {
                            cache_.updateTrade(tick.symbol_id,tick.last_trade_price,tick.trade_qty,tick.timestamp_ns);
                        }

                        // Quote update
                        cache_.updateBid(tick.symbol_id,tick.bid_price,tick.bid_qty,tick.timestamp_ns);
                        cache_.updateAsk(tick.symbol_id,tick.ask_price,tick.ask_qty,tick.timestamp_ns);

                        });

                    // Feed raw bytes to parser
                    // parser_.consume(rx_buffer.data(), bytes,[&](const Tick& tick) {
                    //     // Trade update
                    //     if (tick.trade_qty > 0) {
                    //         cache_.updateTrade(tick.symbol_id,tick.last_trade_price,tick.trade_qty,tick.timestamp_ns);
                    //     }

                    //     // Quote update
                    //     cache_.updateBid(tick.symbol_id,tick.bid_price,tick.bid_qty,tick.timestamp_ns);
                    //     cache_.updateAsk(tick.symbol_id,tick.ask_price,tick.ask_qty,tick.timestamp_ns);
                    //     });
                    
                }
                else if (bytes == 0) {
                    std::cout << "[feed] Server closed connection\n";
                    socket_.disconnect();
                    // epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,socket_.fd(), nullptr); socket_.socket_fd()

                    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,socket_.socket_fd(), nullptr);

                    if (!connect_with_retry())
                        running_ = false;
                    else
                        setup_epoll();
                    break;
                }
                else { // EAGAIN / EWOULDBLOCK
                    break;
                }
            }
        }
    }

    shutdown();
}

void FeedHandler::shutdown() {
    if (epoll_fd_ >= 0)
        close(epoll_fd_);
    socket_.disconnect();
}

// -------- Entry Point --------

// int main() {
//     FeedHandler handler("127.0.0.1", 9876, 500);
//     handler.run();
//     return 0;
// }



int main() {
    constexpr size_t NUM_SYMBOLS = 500;

    // Shared lock-free cache
    LockFreeSymbolCache cache(NUM_SYMBOLS);

    // Feed handler (writer)
    FeedHandler handler("127.0.0.1", 9876, cache);

    // Visualizer (reader)
    Visualizer vis(cache, NUM_SYMBOLS);

    // UI thread
    std::thread ui([&] {
        vis.run();
    });

    // Network + parsing thread (main thread)
    handler.run();

    // Shutdown UI cleanly
    vis.stop();
    ui.join();

    return 0;
}