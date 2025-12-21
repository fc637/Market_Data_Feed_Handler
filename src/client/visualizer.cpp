// src/client/visualizer.cpp

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "header.h" 
#include "protocol.h"   // MarketState, LatencyTracker

// Forward declaration (no header)
class LockFreeSymbolCache;



// ---------------- Implementation ----------------

void Visualizer::setup_stdin() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void Visualizer::restore_stdin() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
}

void Visualizer::clear_screen() {
    std::cout << "\033[2J\033[H";
}

void Visualizer::handle_input() {
    char c;
    if (read(STDIN_FILENO, &c, 1) > 0) {
        if (c == 'q') {
            running_ = false;
        }
        if (c == 'r') {
            std::cout << "\n[visualizer] Stats reset not implemented yet\n";
        }
    }
}

void Visualizer::print_header(uint64_t msg_count) {
    auto now = std::chrono::steady_clock::now();
    auto uptime =
        std::chrono::duration_cast<std::chrono::seconds>(now - start_time_).count();

    std::cout << "=== NSE Market Data Feed Handler ===\n";
    std::cout << "Uptime: " << uptime << "s"
              << " | Messages: " << msg_count
              << " | Rate: ~" << msg_count / std::max<uint64_t>(1, uptime)
              << " msg/s\n\n";
}

void Visualizer::print_table(const std::vector<size_t>& top) {
    std::cout << "Symbol  Bid        Ask        LTP        Updates\n";
    std::cout << "-------------------------------------------------\n";

    for (size_t id : top) {
        MarketState s;
        if (!cache_.getSnapshot(id, s))
            continue;

        std::cout << id << "   "
                  << s.best_bid << "   "
                  << s.best_ask << "   "
                  << s.last_traded_price << "   "
                  << s.update_count << "\n";
    }
}

void Visualizer::render() {
    clear_screen();

    uint64_t total_updates = 0;
    std::vector<size_t> ids(num_symbols_);

    for (size_t i = 0; i < num_symbols_; ++i) {
        ids[i] = i;
        MarketState s;
        if (cache_.getSnapshot(i, s))
            total_updates += s.update_count;
    }

    // Top 20 symbols by update count
    std::partial_sort(ids.begin(), ids.begin() + 20, ids.end(),
        [&](size_t a, size_t b) {
            MarketState sa, sb;
            cache_.getSnapshot(a, sa);
            cache_.getSnapshot(b, sb);
            return sa.update_count > sb.update_count;
        });

    print_header(total_updates);
    print_table({ids.begin(), ids.begin() + std::min<size_t>(20, ids.size())});

    std::cout << "\nStatistics:\n";
    std::cout << "Latency p50: " << LatencyTracker::instance().p50() / 1000 << " us\n";
    std::cout << "Latency p99: " << LatencyTracker::instance().p99() / 1000 << " us\n";

    std::cout << "\nPress 'q' to quit\n";
}

void Visualizer::run() {
    while (running_) {
        handle_input();
        render();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    restore_stdin();
}
