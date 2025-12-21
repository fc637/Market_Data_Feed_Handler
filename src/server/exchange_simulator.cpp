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

