// include/exchange_simulator.h
#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <random>
#include <chrono>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include "protocol.h"  // for Tick struct

using namespace std;

/************************************************************************************************ */
//  USED as SEParate 
struct SymbolState {
    double price;
    double volatility;
    double drift;
    double spread;
    uint64_t seq_no;
};

class TickGenerator {
public:
    TickGenerator(size_t num_symbols);

    // Generate one tick for a symbol
    bool generate(uint16_t symbol_id, Tick& out);

private:
    std::unordered_map<uint16_t, SymbolState> symbols_;
    std::mt19937_64 rng_;
};

class ClientManager {
public:
    ClientManager();

    void add_listener(int listen_fd);
    void handle_events(int listen_fd);
    void broadcast(const void* data, size_t len);

private:
    int epoll_fd_;
    std::vector<int> clients_;

    void set_nonblocking(int fd);
    void handle_new_connection(int listen_fd);
    void disconnect(int fd);
};


class ExchangeSimulator {
public:
    // Initialize with port and number of symbols
    ExchangeSimulator(uint16_t port, size_t num_symbols = 100);

    // Start accepting connections
    void start();

    // Main event loop
    void run();

    // Configuration
    void set_tick_rate(uint32_t ticks_per_second);
    void enable_fault_injection(bool enable);

private:
    // Configuration
    uint16_t port_;
    size_t num_symbols_;
    uint32_t tick_rate_{10000};
    bool fault_injection_{false};

    // Networking
    int listen_fd_{-1};
    ClientManager client_manager_;

    // Market data
    TickGenerator tick_generator_;
};
/***********************************************************************************************/

/******************************************************************************************************************************************** */
// USED IN SINGLE 
// class ExchangeSimulator {
// public:
//     ExchangeSimulator(uint16_t port, size_t num_symbols = 100);
//     void start();
//     void run();
//     void set_tick_rate(uint32_t ticks_per_second);
//     void enable_fault_injection(bool enable);

// private:
//     void handle_new_connection();
//     void generate_tick(uint16_t symbol_id);
//     void broadcast_message(const void* data, size_t len);
//     void handle_client_disconnect(int client_fd);
//     void set_nonblocking(int fd);

// private:
//     uint16_t port_;
//     size_t num_symbols_;
//     uint32_t tick_rate_ = 10000; // default 10k msgs/sec
//     bool fault_injection_ = false;

//     int listen_fd_;
//     int epoll_fd_;
//     vector<int> client_fds_;
//     unordered_map<uint32_t, SymbolState> symbols_;

//     mt19937_64 rng_;
// };
