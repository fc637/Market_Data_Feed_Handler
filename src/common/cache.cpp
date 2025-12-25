#include <stdexcept>
#include "protocol.h"
#include <vector>


/* ----------------- Internal atomic state ----------------- */
// struct alignas(64) AtomicMarketState {
//     std::atomic<uint64_t> version{0};
//     MarketState state;
// };

struct alignas(64) AtomicMarketState {
    std::atomic<uint64_t> seq;
    MarketState data{};
};

/* ================= PIMPL DEFINITION ================= */

// THIS LINE IS THE FIX
struct LockFreeSymbolCache::LockFreeSymbolCacheImpl {
    explicit LockFreeSymbolCacheImpl(size_t n)
        : symbols(n) {
            for (auto& s : symbols) {
                s.data = MarketState{};
                s.seq.store(0, std::memory_order_relaxed);
            }
        }

    std::vector<AtomicMarketState> symbols;
};

/* ----------------- helpers ----------------- */
// static inline void begin_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release);
// }

// static inline void end_write(AtomicMarketState& s) {
//     s.version.fetch_add(1, std::memory_order_release);
// }

static inline void begin_write(AtomicMarketState& s) {
    s.seq.fetch_add(1, std::memory_order_release); // odd = write begin
}

static inline void end_write(AtomicMarketState& s) {
    s.seq.fetch_add(1, std::memory_order_release); // even = write end
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

    s.data.best_bid = price;
    s.data.bid_quantity = qty;
    s.data.last_update_time = ts;
    s.data.update_count++;

    // s.state.best_bid = price;
    // s.state.bid_quantity = qty;
    // s.state.last_update_time = ts;
    // s.state.update_count++;

    end_write(s);
}

void LockFreeSymbolCache::updateAsk(
    uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

    // auto& s = symbols_[symbol];
    auto& s = impl_->symbols[symbol];
    begin_write(s);

    s.data.best_ask = price;
    s.data.ask_quantity = qty;
    s.data.last_update_time = ts;
    s.data.update_count++;    
    // s.state.best_ask = price;
    // s.state.ask_quantity = qty;
    // s.state.last_update_time = ts;
    // s.state.update_count++;

    end_write(s);
}


// ---- Reader API ----
// bool LockFreeSymbolCache::getSnapshot(
//     uint32_t symbol, MarketState& out) const {

//     // const auto& s = symbols_[symbol];
//     const auto& s = impl_->symbols[symbol];

//     while (true) {
//         uint64_t v1 = s.version.load(std::memory_order_acquire);
//         if (v1 & 1) continue;  // writer active

//         out = s.state;         // plain copy

//         uint64_t v2 = s.version.load(std::memory_order_acquire);
//         if (v1 == v2)
//             return true;       // consistent snapshot
//     }
// }

// bool LockFreeSymbolCache::getSnapshot(
//     uint32_t symbol, MarketState& out) const {

//     const auto& s = impl_->symbols[symbol];

//     while (true) {
//         uint64_t start = s.seq.load(std::memory_order_acquire);
//         if (start & 1) continue;   // writer in progress

//         out = s.data;              // copy snapshot

//         uint64_t end = s.seq.load(std::memory_order_acquire);
//         if (start == end)
//             return true;
//     }
// }

bool LockFreeSymbolCache::getSnapshot(
    uint32_t symbol, MarketState& out) const {

    const auto& s = impl_->symbols[symbol];

    while (true) {
        uint64_t start = s.seq.load(std::memory_order_acquire);
        if (start & 1) continue;

        out = s.data;

        uint64_t end = s.seq.load(std::memory_order_acquire);
        if (start == end)
            return true;
    }
}size_t LockFreeSymbolCache::size() const {
    return impl_->symbols.size();
}

void LockFreeSymbolCache::updateTrade(
    uint32_t symbol, double price, uint32_t qty, uint64_t ts) {

    // auto& s = impl_->symbols[symbol];
    auto& s = impl_->symbols[symbol];
    begin_write(s);

    s.data.last_traded_price = price;
    s.data.last_traded_quantity = qty;
    s.data.last_update_time = ts;
    s.data.update_count++;

    // s.state.last_traded_price = price;
    // s.state.last_traded_quantity = qty;
    // s.state.last_update_time = ts;
    // s.state.update_count++;

    end_write(s);
}
