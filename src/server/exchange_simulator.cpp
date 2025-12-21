// src/server/exchange_simulator.cpp
#include "exchange_simulator.h"
#include "protocol.h"  // for Tick struct
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cstring>
#include <algorithm>
#include <thread>
#include <mutex>
using namespace std;

ExchangeSimulator::ExchangeSimulator(uint16_t port, size_t num_symbols)
    : port_(port),num_symbols_(num_symbols),tick_generator_(num_symbols) {}

void ExchangeSimulator::set_tick_rate(uint32_t ticks_per_second) {
    if (ticks_per_second > 0)
        tick_rate_ = ticks_per_second;
}

void ExchangeSimulator::enable_fault_injection(bool enable) {
    fault_injection_ = enable;
}

void ExchangeSimulator::start() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd_, SOMAXCONN);

    client_manager_.add_listener(listen_fd_);
    run();
}

void ExchangeSimulator::run() {
    using clock = std::chrono::high_resolution_clock;
    auto tick_interval =
        std::chrono::microseconds(1000000 / tick_rate_);

    while (true) {
        auto loop_start = clock::now();

        // Handle client connections & disconnects
        client_manager_.handle_events(listen_fd_);

        // Generate ticks for all symbols
        for (uint16_t i = 0; i < num_symbols_; ++i) {
            Tick tick;
            if (tick_generator_.generate(i, tick)) {
                client_manager_.broadcast(&tick, sizeof(Tick));
            }
        }

        // Rate control
        auto elapsed = clock::now() - loop_start;
        if (elapsed < tick_interval) {
            std::this_thread::sleep_for(tick_interval - elapsed);
        }
    }
}

/*******************************************************************************************************/
// ExchangeSimulator::ExchangeSimulator(uint16_t port, size_t num_symbols)
//     : port_(port), num_symbols_(num_symbols), rng_(random_device{}()) {
//         uniform_real_distribution<double> price_dist(100, 5000);
//         uniform_real_distribution<double> vol_dist(0.01, 0.06);

//         for (uint32_t i = 0; i < num_symbols_; ++i) {
//             SymbolState s;
//             s.price = price_dist(rng_);
//             s.volatility = vol_dist(rng_);
//             s.drift = 0.0; // neutral
//             s.spread = 0.0;
//             s.seq_no = 0;
//             symbols_[i] = s;
//         }
//     }



/*******************************************************************************************************/


// ExchangeSimulator::ExchangeSimulator(uint16_t port, size_t num_symbols)
//     : port_(port), num_symbols_(num_symbols), rng_(random_device{}())
// {
//     uniform_real_distribution<double> price_dist(100, 5000);
//     uniform_real_distribution<double> vol_dist(0.01, 0.06);

//     for (uint32_t i = 0; i < num_symbols_; ++i) {
//         SymbolState s;
//         s.price = price_dist(rng_);
//         s.volatility = vol_dist(rng_);
//         s.drift = 0.0; // neutral
//         s.spread = 0.0;
//         s.seq_no = 0;
//         symbols_[i] = s;
//     }
// }

// ---------------- Configuration ----------------
// void ExchangeSimulator::set_tick_rate(uint32_t ticks_per_second) {
//     if (ticks_per_second > 0) tick_rate_ = ticks_per_second;
// }

// void ExchangeSimulator::enable_fault_injection(bool enable) {
//     fault_injection_ = enable;
// }

// // ---------------- Server Helpers ----------------
// void ExchangeSimulator::set_nonblocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

// void ExchangeSimulator::handle_new_connection() {
//     while (true) {
//         sockaddr_in client_addr{};
//         socklen_t client_len = sizeof(client_addr);
//         int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &client_len);
//         if (client_fd < 0) {
//             if (errno == EAGAIN || errno == EWOULDBLOCK) break;
//             perror("accept");
//             break;
//         }
//         set_nonblocking(client_fd);
//         client_fds_.push_back(client_fd);
//         epoll_event ev{};
//         ev.events = EPOLLIN | EPOLLET;
//         ev.data.fd = client_fd;
//         epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
//     }
// }

// void ExchangeSimulator::handle_client_disconnect(int client_fd) {
//     epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
//     close(client_fd);
//     client_fds_.erase(remove(client_fds_.begin(), client_fds_.end(), client_fd), client_fds_.end());
// }

// // ---------------- Tick Generation ----------------
// void ExchangeSimulator::generate_tick(uint16_t symbol_id) {
//     auto &s = symbols_[symbol_id];

//     double dt = 0.001;
//     normal_distribution<double> normal(0.0, 1.0);
//     double dW = normal(rng_) * sqrt(dt);
//     s.price += s.drift * s.price * dt + s.volatility * s.price * dW;

//     s.spread = s.price * (0.0005 + ((double)rand() / RAND_MAX) * 0.0015);
//     double bid = s.price - s.spread/2.0;
//     double ask = s.price + s.spread/2.0;

//     Tick tick;
//     tick.timestamp_ns = chrono::duration_cast<chrono::nanoseconds>(
//         chrono::high_resolution_clock::now().time_since_epoch()).count();
//     tick.symbol_id = symbol_id;
//     tick.seq_no = ++s.seq_no;

//     if ((rand() % 100) < 30) { // 30% trade
//         tick.last_trade_price = s.price;
//         tick.trade_qty = 50;
//         tick.bid_price = 0; tick.ask_price = 0;
//         tick.bid_qty = 0; tick.ask_qty = 0;
//     } else { // 70% quote
//         tick.bid_price = bid;
//         tick.ask_price = ask;
//         tick.last_trade_price = s.price;
//         tick.bid_qty = 100;
//         tick.ask_qty = 100;
//         tick.trade_qty = 0;
//     }

//     if (fault_injection_ && (rand() % 1000) < 5) return; // optional fault

//     broadcast_message(&tick, sizeof(Tick));
// }

// // ---------------- Broadcast ----------------
// void ExchangeSimulator::broadcast_message(const void* data, size_t len) {
//     (void)data;
//     (void)len;

//     std::string msg = "SYMBOL=ABC PRICE=123.45\n";


//     // for (int fd : client_fds_) {
//     //     ssize_t n = send(fd, data, len, 0);
//     //     if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
//     //         handle_client_disconnect(fd);
//     //     }
//     // }

//     for (int fd : client_fds_) {
//         ssize_t n = send(fd, msg.data(), msg.size(), 0);
//         if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
//             handle_client_disconnect(fd);
//         }
//     }
// }

// // ---------------- Event Loop ----------------
// void ExchangeSimulator::run() {
//     epoll_event events[1024];
//     while (true) {
//         int n = epoll_wait(epoll_fd_, events, 1024, 1);
//         for (int i = 0; i < n; ++i) {
//             if (events[i].data.fd == listen_fd_) handle_new_connection();
//             else if (events[i].events & (EPOLLHUP | EPOLLERR))
//                 handle_client_disconnect(events[i].data.fd);
//         }

//         for (uint16_t i = 0; i < num_symbols_; ++i)
//             generate_tick(i);

//         this_thread::sleep_for(chrono::microseconds(1000000 / tick_rate_));
//     }
// }

// // ---------------- Start ----------------
// void ExchangeSimulator::start() {
//     listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
//     set_nonblocking(listen_fd_);

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = INADDR_ANY;
//     addr.sin_port = htons(port_);

//     if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
//     if (listen(listen_fd_, SOMAXCONN) < 0) { perror("listen"); exit(1); }

//     epoll_fd_ = epoll_create1(0);
//     epoll_event ev{};
//     ev.events = EPOLLIN;
//     ev.data.fd = listen_fd_;
//     epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

//     run();
// }
/**********************************************************************************************************8 */
/*************************************************************************************************88 */

// ExchangeSimulator::ExchangeSimulator(uint16_t port, size_t num_symbols)
//     : port_(port), num_symbols_(num_symbols), rng_(random_device{}())
// {
//     // Initialize symbols with random starting price, drift, volatility
//     uniform_real_distribution<double> price_dist(100, 5000);
//     uniform_real_distribution<double> vol_dist(0.01, 0.06);
//     for (uint32_t i = 0; i < num_symbols_; ++i) {
//         SymbolState s;
//         s.price = price_dist(rng_);
//         s.volatility = vol_dist(rng_);
//         s.drift = 0.0; // neutral market for now
//         symbols_[i] = s;
//     }
// }

// // ---------------- Configuration Functions ----------------

// void ExchangeSimulator::set_tick_rate(uint32_t ticks_per_second) {
//     if (ticks_per_second == 0) {
//         cerr << "Tick rate cannot be zero. Using default: 10000" << endl;
//         return;
//     }
//     tick_rate_ = ticks_per_second;
//     cout << "Tick rate set to " << tick_rate_ << " messages/sec" << endl;
// }

// void ExchangeSimulator::enable_fault_injection(bool enable) {
//     fault_injection_ = enable;
//     cout << "Fault injection " << (enable ? "enabled" : "disabled") << endl;
// }

// // ---------------- Server Helper Functions ----------------

// void ExchangeSimulator::set_nonblocking(int fd) {
//     int flags = fcntl(fd, F_GETFL, 0);
//     fcntl(fd, F_SETFL, flags | O_NONBLOCK);
// }

// void ExchangeSimulator::handle_new_connection() {
//     while (true) {
//         sockaddr_in client_addr{};
//         socklen_t client_len = sizeof(client_addr);
//         int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &client_len);
//         if (client_fd < 0) {
//             if (errno == EAGAIN || errno == EWOULDBLOCK)
//                 break; // no more clients
//             perror("accept");
//             break;
//         }
//         set_nonblocking(client_fd);

//         client_fds_.push_back(client_fd);

//         epoll_event ev{};
//         ev.events = EPOLLIN | EPOLLOUT | EPOLLET; // edge-triggered
//         ev.data.fd = client_fd;
//         epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

//         cout << "New client connected, fd=" << client_fd << endl;
//     }
// }

// void ExchangeSimulator::handle_client_disconnect(int client_fd) {
//     epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
//     close(client_fd);
//     client_fds_.erase(remove(client_fds_.begin(), client_fds_.end(), client_fd), client_fds_.end());
//     cout << "Client disconnected, fd=" << client_fd << endl;
// }

// // ---------------- Tick Generation ----------------

// void ExchangeSimulator::generate_tick(uint16_t symbol_id) {
//     auto &s = symbols_[symbol_id];

//     double dt = 0.001; // 1ms
//     normal_distribution<double> normal(0.0, 1.0);
//     double dW = normal(rng_) * sqrt(dt);

//     double dS = s.drift * s.price * dt + s.volatility * s.price * dW;
//     s.price += dS;

//     // Maintain realistic bid-ask spread
//     s.spread = s.price * (0.0005 + ((double)rand() / RAND_MAX) * 0.0015);
//     double bid = s.price - s.spread / 2.0;
//     double ask = s.price + s.spread / 2.0;

//     Tick tick;
//     tick.timestamp_ns = chrono::duration_cast<chrono::nanoseconds>(
//                             chrono::high_resolution_clock::now().time_since_epoch())
//                             .count();
//     tick.symbol_id = symbol_id;

//     // 70% Quote / 30% Trade
//     bool is_trade = (rand() % 100) < 30; // 30% chance trade
//     if (is_trade) {
//         tick.last_trade_price = s.price;
//         tick.trade_qty = 50;
//         tick.bid_price = 0;  // optional, only quote updates
//         tick.ask_price = 0;
//         tick.bid_qty = 0;
//         tick.ask_qty = 0;
//     } else {
//         tick.bid_price = bid;
//         tick.ask_price = ask;
//         tick.last_trade_price = s.price;
//         tick.bid_qty = 100;
//         tick.ask_qty = 100;
//         tick.trade_qty = 0;
//     }

//     tick.seq_no = ++s.seq_no;

//     // Optional fault injection
//     if (fault_injection_ && (rand() % 1000) < 5) {
//         // skip broadcasting for this tick
//         return;
//     }

//     broadcast_message(&tick, sizeof(Tick));
// }

// // ---------------- Broadcast ----------------

// void ExchangeSimulator::broadcast_message(const void* data, size_t len) {
//     for (int fd : client_fds_) {
//         ssize_t n = send(fd, data, len, 0);
//         if (n < 0) {
//             if (errno != EAGAIN && errno != EWOULDBLOCK) {
//                 handle_client_disconnect(fd);
//             }
//         }
//     }
// }

// // ---------------- Server Loop ----------------

// void ExchangeSimulator::run() {
//     epoll_event events[1024];

//     while (true) {
//         int n = epoll_wait(epoll_fd_, events, 1024, 1);
//         for (int i = 0; i < n; ++i) {
//             if (events[i].data.fd == listen_fd_) {
//                 handle_new_connection();
//             } else {
//                 if (events[i].events & (EPOLLHUP | EPOLLERR)) {
//                     handle_client_disconnect(events[i].data.fd);
//                 }
//             }
//         }

//         // Generate ticks for all symbols
//         for (uint16_t i = 0; i < num_symbols_; ++i) {
//             generate_tick(i);
//         }

//         // Control tick rate
//         this_thread::sleep_for(chrono::microseconds(1000000 / tick_rate_));
//     }
// }

// // ---------------- Start Server ----------------

// void ExchangeSimulator::start() {
//     listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
//     set_nonblocking(listen_fd_);

//     sockaddr_in addr{};
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = INADDR_ANY;
//     addr.sin_port = htons(port_);

//     if (bind(listen_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
//         perror("bind failed");
//         exit(1);
//     }

//     if (listen(listen_fd_, SOMAXCONN) < 0) {
//         perror("listen failed");
//         exit(1);
//     }

//     epoll_fd_ = epoll_create1(0);
//     epoll_event ev{};
//     ev.events = EPOLLIN;
//     ev.data.fd = listen_fd_;
//     epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

//     cout << "ExchangeSimulator listening on port " << port_ << endl;
//     run();
// }