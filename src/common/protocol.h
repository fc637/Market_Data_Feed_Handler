// src/common/protocol.h
#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
struct MarketState {
    double   best_bid = 0;
    double   best_ask = 0;
    uint32_t bid_quantity = 0;
    uint32_t ask_quantity = 0;
    double   last_traded_price = 0;
    uint32_t last_traded_quantity = 0;
    uint64_t last_update_time = 0;
    uint64_t update_count = 0;
};
// Shared Tick structure between server and client
struct Tick {
    uint64_t timestamp_ns;       // Nanoseconds since epoch
    uint32_t symbol_id;          // Symbol ID (0 - num_symbols-1)
    double bid_price;            // Best bid price
    double ask_price;            // Best ask price
    double last_trade_price;     // Last traded price
    uint32_t bid_qty;            // Best bid quantity
    uint32_t ask_qty;            // Best ask quantity
    uint32_t trade_qty;          // Last trade quantity
    uint64_t seq_no;             // Sequence number (strictly increasing)
};


class MemoryPool {
public:
    MemoryPool(size_t block_size, size_t block_count);

    void* allocate();
    void deallocate(void* ptr);

private:
    size_t block_size_;
    std::vector<void*> free_list_;
    std::vector<char> storage_;
};



class LatencyTracker {
public:
    static LatencyTracker& instance();
    void record_kernel_to_user(uint64_t ns);
     void record_userspace(uint64_t ns);   // Add This
    uint64_t p50() const;
    uint64_t p99() const;

private:
    LatencyTracker() = default;

    mutable std::mutex mtx_;
    std::vector<uint64_t> samples_;
    // std::vector<uint64_t> samples_;
};

class LockFreeSymbolCache {
public:
    explicit LockFreeSymbolCache(size_t num_symbols);
      ~LockFreeSymbolCache();

    // Writer API (single thread)
    void updateBid(uint32_t symbol, double price, uint32_t qty, uint64_t ts);
    void updateAsk(uint32_t symbol, double price, uint32_t qty, uint64_t ts);
    void updateTrade(uint32_t symbol, double price, uint32_t qty, uint64_t ts);

    // Reader API (lock-free)
    bool getSnapshot(uint32_t symbol, MarketState& out) const;

private:
    // std::vector<AtomicMarketState> symbols_;
    // private:
    // struct AtomicMarketState; 
    struct LockFreeSymbolCacheImpl;          // forward declare
    std::unique_ptr<LockFreeSymbolCacheImpl> impl_;
};

// class SymbolCache {
// public:
//     explicit SymbolCache(size_t num_symbols);

//     void update(const Tick& tick);
//     const Tick& last(uint32_t symbol_id) const;

// private:
//     std::vector<Tick> last_ticks_;
// };