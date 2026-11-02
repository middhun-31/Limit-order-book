# Low-Latency Limit Order Book Engine

A compact C++ implementation of a limit order book designed for fast matching, cancellation lookup, and trade reporting. The engine is built around a memory-pool-backed order book, per-price level queues, and a lock-free ring buffer for asynchronous trade publication.

## Overview

This project models the core behavior of a trading venue:

- matching incoming buy/sell orders against the best available prices
- maintaining a bid/ask book across price levels
- supporting cancellation by order ID
- emitting trade events to a lock-free queue for downstream consumption
- benchmarking passive placement and aggressive matching throughput

The code is intentionally focused on low-latency data structures rather than a full exchange stack, making it useful as a reference implementation or performance-oriented prototype.

## Key components

- `include/order.h` — order definitions, side, type, and time-in-force enums
- `include/PriceLevel.h` — per-price queue and volume accounting
- `include/OrderPool.h` — preallocated memory pool for order slots
- `include/OrderBook.h` — matching logic, best bid/ask tracking, cancellation, and trade queue integration
- `include/RingBuffer.h` — fixed-capacity lock-free ring buffer for trade events
- `src/main.cpp` — a demo scenario exercising order matching and IOC behavior
- `src/benchmark.cpp` — Google Benchmark tests for passive placement and aggressive matching

## Features

- O(1)-style cancellation lookup via an order ID map
- price-based routing into paged memory regions for compact address calculation
- lock-free trade publication using a ring buffer with cache-line separation
- support for `GTC`, `IOC`, and `FOK` semantics in the order model
- low-level performance instrumentation with `-O3` and architecture-tuned compile flags

## Build requirements

- CMake 3.20+
- C++20 compatible compiler
- pthread support
- Google Benchmark library

## Building

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

This creates:

- `build/MatchingEngine` — executable demo for the order book
- `build/LatencyBenchmark` — benchmark binary using Google Benchmark

## Running the demo

```bash
./build/MatchingEngine
```

The sample program in `src/main.cpp` creates a book, adds passive ask/bid orders, crosses the spread with an aggressive buyer, drains trade events from the ring buffer, and validates IOC behavior.

## Running benchmarks

```bash
./build/LatencyBenchmark --benchmark_min_time=0.1s
```

The benchmark suite measures:

- passive order placement throughput
- aggressive matching throughput while writing to the lock-free queue

## Benchmark results

Measured on the current machine with the local build configuration:

```text
2026-11-02T15:06:44+00:00
Running ./build/LatencyBenchmark
Run on (16 X 5090.91 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)

Benchmark                            Time             CPU   Iterations  items_per_second
-----------------------------------------------------------------------------------------
BM_PassivePlacement/30000       472804 ns       472638 ns          300   63.4735M/s
BM_AggressiveMatching/30000     470281 ns       469375 ns          307   63.9147M/s
```

Per-order latency, computed as benchmark time divided by 30,000 orders per iteration:

- `BM_PassivePlacement/30000`: approximately 15.76 ns per order
- `BM_AggressiveMatching/30000`: approximately 15.68 ns per order

> Note: the benchmark output includes the library warning that the build is a debug build and CPU scaling is enabled, so these numbers are representative but not absolute peak throughput.

## Example execution flow

The demo simulates a simple matching scenario:

1. add passive orders at prices such as 150 and 151
2. add a buyer at 151 that crosses the spread
3. consume trade events from the ring buffer
4. test an IOC order to ensure unmatched volume is cancelled immediately

A typical trade output looks like:

```text
TRADE EXECUTED: Buyer ID 301 bought 200 shares at Price $150
TRADE EXECUTED: Buyer ID 301 bought 50 shares at Price $151
IOC Triggered: 550 shares of Order 401 cancelled.
```

## Notes

This repository is best viewed as a focused engine prototype rather than a full exchange or production-grade matching system. The implementation intentionally emphasizes speed and memory efficiency, and the benchmark harness is useful for understanding throughput characteristics under different order patterns.

## Typical project layout

```text
.
├── CMakeLists.txt
├── README.md
├── include/
│   ├── OrderBook.h
│   ├── OrderPool.h
│   ├── PriceLevel.h
│   ├── RingBuffer.h
│   └── order.h
├── src/
│   ├── benchmark.cpp
│   └── main.cpp
├── tests/
└── build/
```

## Why this design matters

The implementation uses a few standard latency-optimizing patterns:

- a fixed-size order pool to avoid dynamic allocation during matching
- price-level storage to keep best bid/ask lookups inexpensive
- a lock-free publication queue to decouple matching from downstream consumers
- cache-aware alignment for the ring buffer to reduce false sharing

That combination makes it suitable for exploring how a low-latency matching engine is structured, while still being simple enough to study and extend.
