// client/parser.cpp

#include <cstdint>
#include <cstring>
#include <iostream>
#include <atomic>
#include "header.h" 
#include "../common/protocol.h"
// #include "../common/latency_tracker.h"

namespace {

// Message types
constexpr uint16_t MSG_TRADE = 0x01;
constexpr uint16_t MSG_QUOTE = 0x02;
constexpr uint16_t MSG_HEARTBEAT = 0x03;

inline uint32_t xor_checksum(const uint8_t* data, size_t len) {
    uint32_t x = 0;
    for (size_t i = 0; i < len; ++i)
        x ^= data[i];
    return x;
}

} // anonymous namespace

MarketDataParser::MarketDataParser()
    : write_pos_(0),
      read_pos_(0),
      last_seq_(0) {}

void MarketDataParser::consume(const uint8_t* data,
                               size_t len,
                               TickCallback on_tick) {
    if (len == 0) return;

    if (write_pos_ + len > MAX_BUFFER) {
        std::cerr << "[PARSER] Buffer overflow risk, resetting\n";
        reset();
        return;
    }

    std::memcpy(buffer_ + write_pos_, data, len);
    write_pos_ += len;

    parse_loop(on_tick);
}

void MarketDataParser::reset() {
    write_pos_ = 0;
    read_pos_ = 0;
}

void MarketDataParser::parse_loop(TickCallback on_tick) {
    while (true) {
        size_t available = write_pos_ - read_pos_;
        if (available < HEADER_SIZE + CHECKSUM_SIZE)
            return;

        const uint8_t* ptr = buffer_ + read_pos_;

        uint16_t type;
        uint32_t seq;
        uint64_t ts;
        uint16_t sym;

        std::memcpy(&type, ptr, 2);
        std::memcpy(&seq, ptr + 2, 4);
        std::memcpy(&ts,  ptr + 6, 8);
        std::memcpy(&sym, ptr + 14, 2);

        size_t payload_size = 0;

        switch (type) {
        case MSG_TRADE: payload_size = 12; break;
        case MSG_QUOTE: payload_size = 24; break;
        case MSG_HEARTBEAT: payload_size = 0; break;
        default:
            drop_bytes(1);
            continue;
        }

        size_t msg_size = HEADER_SIZE + payload_size + CHECKSUM_SIZE;
        if (available < msg_size) return;

        uint32_t expected;
        std::memcpy(&expected, ptr + msg_size - 4, 4);

        if (xor_checksum(ptr, msg_size - 4) != expected) {
            drop_bytes(1);
            continue;
        }

        Tick tick{};
        tick.timestamp_ns = ts;
        tick.symbol_id = sym;
        tick.seq_no = seq;

        const uint8_t* payload = ptr + HEADER_SIZE;

        if (type == MSG_TRADE) {
            std::memcpy(&tick.last_trade_price, payload, 8);
            std::memcpy(&tick.trade_qty, payload + 8, 4);
        } else if (type == MSG_QUOTE) {
            std::memcpy(&tick.bid_price, payload, 8);
            std::memcpy(&tick.bid_qty, payload + 8, 4);
            std::memcpy(&tick.ask_price, payload + 12, 8);
            std::memcpy(&tick.ask_qty, payload + 20, 4);
        }
        // Latency tracking (end of parse)
        LatencyTracker::instance().record_userspace(ts);
        on_tick(tick);
        read_pos_ += msg_size;
    }

    compact();
}

void MarketDataParser::drop_bytes(size_t n) {
    read_pos_ += n;
    compact();
}

void MarketDataParser::compact() {
    if (read_pos_ == 0) return;

    if (read_pos_ == write_pos_) {
        read_pos_ = write_pos_ = 0;
        return;
    }

    std::memmove(buffer_,
                 buffer_ + read_pos_,
                 write_pos_ - read_pos_);

    write_pos_ -= read_pos_;
    read_pos_ = 0;
}

