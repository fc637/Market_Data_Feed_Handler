# Geometric Brownian Motion (GBM) Tick Generator

## Overview

GBM is used to simulate realistic price movements for multiple symbols. The generator produces both **quotes** and **trades** with configurable volatility and drift.

---

## GBM Formula

dS = μ * S * dt + σ * S * dW


Where:

- `S` = current price  
- `μ` = drift (0.0 neutral, +0.05 bull, -0.05 bear)  
- `σ` = volatility (0.02–0.05 typical)  
- `dt` = time step (e.g., 0.001 for millisecond ticks)  
- `dW` = Wiener process (random normal)

### Discretized Update




- `Z ~ N(0,1)` generated using Box-Muller transform

---

## Implementation Details

- 100 symbols with independent GBM streams
- Initial price: ₹100–₹5000
- Volatility randomized: σ ∈ [0.01, 0.06]
- Tick interval derived from configurable tick rate
- Message mix: 70% quotes, 30% trades
- Bid-ask spread: 0.05%–0.2% of mid-price

### Quote Generation

bid = price - spread/2
ask = price + spread/2


- Quantity fixed or randomized as needed
- Sequence number strictly increasing

### Trade Generation

- Trade price near mid-price
- Trade quantity randomized
- Bid/ask fields set to 0

---

## Random Number Generation

- Box-Muller transform used for normal distribution
- Fast and deterministic
- Ensures low-latency, repeatable tick generation

---

## Multi-Symbol Independence

- Each symbol maintains independent:
  - Price state
  - Volatility
  - Random number generator
- Prevents artificial correlation between symbols

---

## Summary

GBM provides:
- Realistic price evolution
- Configurable volatility and drift
- Deterministic and fast computation
- Ideal for **low-latency market data feed testing**

