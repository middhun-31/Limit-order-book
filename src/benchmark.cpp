#include "OrderBook.h"
#include <benchmark/benchmark.h>
#include <vector>

struct MockOrder {
    uint64_t id;
    uint32_t price;
    uint32_t qty;
    Side side;
    OrderType type;
    TimeInForce tif;
};

// Pre-generate orders to isolate the matching engine's speed from random number generation
std::vector<MockOrder> generatePassiveOrders(size_t count) {
    std::vector<MockOrder> orders(count);
    for (size_t i = 0; i < count; ++i) {
        orders[i] = {
            static_cast<uint64_t>(i + 1),
            static_cast<uint32_t>(100 + (i % 50)),
            100,
            (i % 2 == 0) ? Side::Bid : Side::Ask,
            OrderType::LIMIT,
            TimeInForce::GTC
        };
    }
    return orders;
}

std::vector<MockOrder> generateAggressiveOrders(size_t count) {
    std::vector<MockOrder> orders(count);
    for (size_t i = 0; i < count; ++i) {
        orders[i] = {
            static_cast<uint64_t>(i + 1000000),
            150,
            10,
            Side::Bid,
            OrderType::LIMIT,
            TimeInForce::GTC
        };
    }
    return orders;
}

// ============================================================================
// TEST 1: PASSIVE ORDER PLACEMENT (No matches, purely routing and memory pool)
// ============================================================================
static void BM_PassivePlacement(benchmark::State& state) {
    size_t batch_size = state.range(0);
    auto orders = generatePassiveOrders(batch_size);

    for (auto _ : state) {
        // Pause timer while we construct a fresh OrderBook for this iteration
        state.PauseTiming();
        OrderBook book(batch_size + 1000);
        state.ResumeTiming();

        // Blast orders into the engine
        for (const auto& o : orders) {
            book.addOrder(o.id, o.price, o.qty, o.side, o.type, o.tif);
        }
    }

    // Tell Google Benchmark how many items we processed for throughput stats
    state.SetItemsProcessed(state.iterations() * batch_size);
}

// ============================================================================
// TEST 2: AGGRESSIVE MATCHING (Sweeping the book & writing to lock-free queue)
// ============================================================================
static void BM_AggressiveMatching(benchmark::State& state) {
    size_t batch_size = state.range(0);
    auto aggressive_orders = generateAggressiveOrders(batch_size);

    for (auto _ : state) {
        // Pause timer while we setup a book filled with resting liquidity
        state.PauseTiming();
        OrderBook book(batch_size * 2 + 1000);
        for (size_t i = 0; i < batch_size; ++i) {
            book.addOrder(i+1, 150, 100, Side::Ask, OrderType::LIMIT, TimeInForce::GTC);
        }

        // In this benchmark, there is no consumer thread emptying the ring buffer.
        // We must ensure batch_size < 65536, or our engine's Wait Strategy will intentionally deadlock!

        state.ResumeTiming();

        // Blast aggressive buyers that immediately match and write to the Ring Buffer
        for (const auto& o : aggressive_orders) {
            book.addOrder(o.id, o.price, o.qty, o.side, o.type, o.tif);
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}

// Register the benchmarks with a safe batch size of 30,000 orders per iteration
BENCHMARK(BM_PassivePlacement)->Arg(30000);
BENCHMARK(BM_AggressiveMatching)->Arg(30000);

BENCHMARK_MAIN();