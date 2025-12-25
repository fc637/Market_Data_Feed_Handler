# Market Data Feed Handler Assignment

## Low-Latency Multi-Asset Market Data Processor for NSE Co-location

This project implements a **high-performance market data feed handler** designed to simulate and process real-time NSE market data with **ultra-low latency**.  
It focuses on **TCP networking, epoll-based I/O, zero-copy parsing, lock-free data structures, and performance measurement**, similar to real exchange feed handlers used in co-location environments.

---

## Project Overview

The system consists of two main components:

1. **Exchange Simulator (Server)**  
   Simulates an NSE-like exchange by generating realistic market data ticks using **Geometric Brownian Motion (GBM)** and broadcasting them to connected clients.

2. **Feed Handler (Client)**  
   Connects to the exchange, receives binary market data, parses messages efficiently, maintains market state in a lock-free cache, and optionally visualizes live data in the terminal.

---

## High-Level Architecture




┌──────────────────────────────────────────────┐
│ Exchange Simulator (Server)                  │
│ - GBM-based tick generation                  │
│ - 100+ symbols                               │
│ - epoll-based TCP server                     │
│ - 10K–500K msgs/sec                          │
└───────────────────┬─────────────────────────-┘
                    │ Binary Protocol over TCP
                    ▼
┌──────────────────────────────────────────────┐
│ Feed Handler (Client)                        │
│ - epoll-based TCP client                     │
│ - Zero-copy binary parser                    │
│ - Lock-free symbol cache                     │
│ - Terminal visualization & statistics        │
└──────────────────────────────────────────────┘


## Key Features

### Exchange Simulator (Server)
- epoll-based non-blocking TCP server
- Generates realistic market ticks using **Geometric Brownian Motion**
- Supports 100+ symbols with independent price processes
- Randomized volatility and realistic bid-ask spreads
- Message distribution:
  - 70% Quote updates
  - 30% Trade executions
- Strictly increasing sequence numbers
- Nanosecond-precision timestamps
- Configurable tick rate: **10K – 500K messages/sec**
- Graceful handling of client connect/disconnect
- Basic flow control for slow consumers

---

### Feed Handler (Client)
- epoll (edge-triggered) non-blocking TCP client
- Zero-copy binary message parsing
- Handles TCP fragmentation and partial reads
- Automatic reconnect with exponential backoff
- Low-latency socket tuning:
  - TCP_NODELAY
  - Large receive buffers
- Detects:
  - Connection drops
  - Sequence gaps
  - Malformed messages

---

### Binary Protocol Parser
- Zero dynamic allocation in hot path
- Supports Trade, Quote, and Heartbeat messages
- Handles fragmented TCP packets
- Detects sequence gaps
- Validates checksum
- High throughput (100K+ messages/sec)

---

### Lock-Free Market State Cache
- Single writer (feed handler thread)
- Multiple lock-free readers
- Atomic updates without mutex locks
- Consistent snapshots without torn reads
- Cache-line friendly layout for performance

---

### Terminal Visualization (Optional)
- Live terminal view updated every 500ms
- Displays top 20 most active symbols
- Color-coded price movements
- Real-time statistics:
  - Message rate
  - Parser throughput
  - Latency percentiles
  - Sequence gaps
- Non-blocking UI updates

---

### Performance Measurement
- Low-overhead latency tracking
- Fixed-size ring buffer (up to 1 million samples)
- Approximate histogram-based percentile calculation
- Tracks:
  - p50, p95, p99, p999
  - Min / Max / Mean latency
- Optional CSV export for offline analysis



## Build Instructions

### Prerequisites
- Linux (Ubuntu recommended)
- GCC or Clang (C++17 or later)
- CMake
- epoll-compatible Linux kernel

### Build All Components
```bash
cd scripts
./build.sh

Start Exchange Simulator (Server) : Starts the TCP exchange simulator (default: localhost:9876) and begins generating market data.
./scripts/run_server.sh



Start Feed Handler (Client) : Connects to the exchange server, subscribes to symbols, parses incoming data, and updates the market cache.
./scripts/run_client.sh

Run Complete Demo : Runs both the server and client together with live terminal visualization.
./scripts/run_demo.sh


Run Latency Benchmarks 
./scripts/benchmark_latency.sh


Measures:

End-to-end latency
Parser throughput
Percentile statistics
Results are displayed and can be exported to CSV.
