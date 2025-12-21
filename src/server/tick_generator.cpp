#include "exchange_simulator.h"
#include <iostream>
#include <cmath>
#include <chrono>

TickGenerator::TickGenerator(size_t num_symbols)
    : rng_(std::random_device{}())
{
    std::uniform_real_distribution<double> price_dist(100, 5000);
    std::uniform_real_distribution<double> vol_dist(0.01, 0.06);

    for (uint16_t i = 0; i < num_symbols; ++i) {
        symbols_[i] = {
            price_dist(rng_),
            vol_dist(rng_),
            0.0,     // drift
            0.0,     // spread
            0        // seq
        };
    }
}

bool TickGenerator::generate(uint16_t symbol_id, Tick& tick) {
    auto& s = symbols_[symbol_id];

    double dt = 0.001;
    std::normal_distribution<double> normal(0.0, 1.0);
    double dW = normal(rng_) * std::sqrt(dt);

    // GBM
    s.price += s.drift * s.price * dt + s.volatility * s.price * dW;

    // Spread
    s.spread = s.price * (0.0005 + ((double)rand() / RAND_MAX) * 0.0015);
    double bid = s.price - s.spread / 2;
    double ask = s.price + s.spread / 2;

    // Metadata
    tick.timestamp_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    tick.symbol_id = symbol_id;
    tick.seq_no = ++s.seq_no;
    if (symbol_id == 0 && tick.seq_no % 10000 == 0) {
        std::cout << "Generated tick seq=" << tick.seq_no << "\n";
    }

    // 70/30
    if ((rand() % 100) < 30) {
        tick.last_trade_price = s.price;
        tick.trade_qty = 50;
        tick.bid_price = tick.ask_price = 0;
        tick.bid_qty = tick.ask_qty = 0;
    } else {
        tick.bid_price = bid;
        tick.ask_price = ask;
        tick.bid_qty = tick.ask_qty = 100;
        tick.trade_qty = 0;
        tick.last_trade_price = s.price;
    }

    return true;
}
