# Design Document – Market Data Feed Handler

## Overview

This project implements a **low-latency multi-asset market data feed handler** for NSE co-location environments. The system includes:

1. **Exchange Simulator (Server)** – Generates high-frequency market data ticks using Geometric Brownian Motion (GBM).
2. **Feed Handler (Client)** – Receives data via TCP, parses messages, and maintains a lock-free symbol cache.
3. **Visualization (Optional)** – Displays real-time market updates and statistics in the terminal.

The focus is on **low latency**, **high throughput**, and **deterministic performance**.

---

## Architecture

┌───────────────────────────────────────────┐
│ Exchange Simulator (Server) │
│-------------------------------------------│
│ • Generate ticks with GBM │
│ • Multi-symbol (>100) simulation │
│ • TCP server (epoll-based) │
│ • Configurable tick rate (10K–500K msgs/s)│
└───────────────────────┬───────────────────┘
│ Binary Protocol
▼
┌───────────────────────────────────────────┐
│ Feed Handler (Client) │
│-------------------------------------------│
│ • TCP client (epoll-based) │
│ • Zero-copy binary parser │
│ • Lock-free symbol cache │
│ • Terminal visualization & statistics │
└───────────────────────────────────────────┘



---

## Component Breakdown

### 1. Exchange Simulator

**Responsibilities**
- Generate realistic ticks for multiple symbols
- Handle multiple client connections concurrently
- Broadcast messages with minimal blocking
- Maintain sequence numbers and timestamps (ns precision)

**Design Choices**
- `epoll` for scalable I/O multiplexing
- Non-blocking sockets for all clients
- Single thread generating ticks
- Binary protocol to minimize parsing overhead

**Tick Generator**
- 100 symbols with independent GBM processes
- Initial prices: ₹100–₹5000
- Volatility σ ∈ [0.01, 0.06] per symbol
- 70% quotes, 30% trades
- Realistic bid-ask spread (0.05%–0.2%)

---

### 2. Client Feed Handler

**Responsibilities**
- Connect to exchange TCP server
- Parse incoming binary messages efficiently
- Maintain live symbol state in memory
- Provide read access to visualization/UI

**Design Choices**
- Zero-copy parsing for low latency
- Lock-free symbol cache for multi-reader access
- Separate visualization thread to avoid blocking

---

### 3. Lock-Free Symbol Cache

**Purpose**
- Allow single-writer (network thread) and multiple readers (UI, strategies)
- No mutexes in hot paths
- Atomic versioning ensures consistent reads

---

### 4. Visualization Layer

**Responsibilities**
- Terminal-based display of market updates
- Latency statistics monitoring
- Non-blocking access to data cache

---

### Networking Model

- **Server**: epoll monitors listen socket and client sockets; handles new connections and disconnects efficiently.
- **Client**: Non-blocking socket reads; continuous parse → update loop.
- **Broadcasting**: Non-blocking sends; slow clients are skipped to avoid stalling high-frequency ticks.

---

## Binary Protocol

**Header**
- Message type, sequence number, timestamp, symbol ID

**Payload**
- Quote: bid/ask prices and quantities
- Trade: last trade price and quantity

**Design Rationale**
- Minimal size, cache-friendly
- No heap allocation in hot path
- Predictable, deterministic parsing

---

## Latency Considerations

- Avoid locks in critical path
- No dynamic memory allocation during tick processing
- Separate control and data plane

---

## Scalability

- epoll scales to 1000+ connections
- Symbol cache scales linearly with number of symbols
- Tick rate configurable to 500K messages/sec
- Handles slow clients without blocking main generation loop

---

## Fault Handling

- Detect client disconnects
- Clean up file descriptors
- Optional fault injection for testing
- Parser resynchronization on malformed messages

