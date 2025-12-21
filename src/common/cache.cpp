#include <stdexcept>
#include "protocol.h"
#include <vector>

// struct alignas(64) AtomicMarketState {
//     std::atomic<uint64_t> version{0};
//     MarketState state;
// };

// class LockFreeSymbolCacheImpl {
// public:
//     explicit LockFreeSymbolCacheImpl(size_t n) : symbols(n) {}
//     std::vector<AtomicMarketState> symbols;
// };


// static inline void begin_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release);
// }

// static inline void end_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release);
// }

/*******************************************************************************************************8*/

// LockFreeSymbolCache::LockFreeSymbolCache(size_t num_symbols)
//     : symbols_(num_symbols) {}

// ---- Writer helpers ----
// static inline void begin_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release); // make odd
// }

// static inline void end_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release); // make even
// }


// PIMPL storage
// LockFreeSymbolCache::LockFreeSymbolCache(size_t num_symbols)
//     : impl_(new LockFreeSymbolCacheImpl(num_symbols)) {}

/* ----------------- Internal atomic state ----------------- */
struct alignas(64) AtomicMarketState {
    std::atomic<uint64_t> version{0};
    MarketState state;
};

/* ================= PIMPL DEFINITION ================= */

// THIS LINE IS THE FIX
struct LockFreeSymbolCache::LockFreeSymbolCacheImpl {
    explicit LockFreeSymbolCacheImpl(size_t n)
        : symbols(n) {}

    std::vector<AtomicMarketState> symbols;
};

/* ----------------- helpers ----------------- */
static inline void begin_write(AtomicMarketState& s) {
    s.version.fetch_add(1, std::memory_order_release);
}

static inline void end_write(AtomicMarketState& s) {
    s.version.fetch_add(1, std::memory_order_release);
}

/* ================= API IMPLEMENTATION ================= */

LockFreeSymbolCache::LockFreeSymbolCache(size_t num_symbols)
    : impl_(std::make_unique<LockFreeSymbolCacheImpl>(num_symbols)) {}

LockFreeSymbolCache::~LockFreeSymbolCache() = default;

// ---- Writer API ----
void LockFreeSymbolCache::updateBid(
    uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

    // auto& s = symbols_[symbol];
    auto& s = impl_->symbols[symbol];
    begin_write(s);

    s.state.best_bid = price;
    s.state.bid_quantity = qty;
    s.state.last_update_time = ts;
    s.state.update_count++;

    end_write(s);
}

void LockFreeSymbolCache::updateAsk(
    uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

    // auto& s = symbols_[symbol];
    auto& s = impl_->symbols[symbol];
    begin_write(s);

    s.state.best_ask = price;
    s.state.ask_quantity = qty;
    s.state.last_update_time = ts;
    s.state.update_count++;

    end_write(s);
}


// ---- Reader API ----
bool LockFreeSymbolCache::getSnapshot(
    uint32_t symbol, MarketState& out) const {

    // const auto& s = symbols_[symbol];
    const auto& s = impl_->symbols[symbol];

    while (true) {
        uint64_t v1 = s.version.load(std::memory_order_acquire);
        if (v1 & 1) continue;  // writer active

        out = s.state;         // plain copy

        uint64_t v2 = s.version.load(std::memory_order_acquire);
        if (v1 == v2)
            return true;       // consistent snapshot
    }
}

void LockFreeSymbolCache::updateTrade(
    uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

    // auto& s = impl_->symbols[symbol];
    auto& s = impl_->symbols[symbol];
    begin_write(s);

    s.state.last_traded_price = price;
    s.state.last_traded_quantity = qty;
    s.state.last_update_time = ts;
    s.state.update_count++;

    end_write(s);
}

/*******************************************************************************************************8*/
// Without Atomic state 
// void LockFreeSymbolCache::updateTrade(
//     uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

//     auto& s = symbols_[symbol];
//     begin_write(s);

//     s.state.last_traded_price = price;
//     s.state.last_traded_quantity = qty;
//     s.state.last_update_time = ts;
//     s.state.update_count++;

//     end_write(s);
// }

/*******************************************************************************************************8*/
// SymbolCache::SymbolCache(size_t num_symbols)
//     : last_ticks_(num_symbols) {}

// void SymbolCache::update(const Tick& tick) {
//     auto& last = last_ticks_[tick.symbol_id];

//     if (last.seq_no != 0 &&
//         tick.seq_no != last.seq_no + 1) {
//         // gap detected (log later)
//     }

//     last = tick;
// }

// const Tick& SymbolCache::last(uint32_t symbol_id) const {
//     return last_ticks_[symbol_id];
// }