#pragma once
#include "PriceLevel.h"
#include "order.h"
#include "OrderPool.h"
#include "RingBuffer.h"
#include <unordered_map>
#include <vector>
#include <array>
#include <iostream>
#include <immintrin.h>


struct order_tracker {
    uint32_t pool_index;
    PriceLevel* level;                        //pointer which maps tier-1 memory chunks to actual pages
};

constexpr uint32_t PAGE_SHIFT = 10;          // each page shifts after 2^10 =1024 slots
constexpr uint32_t SLOT_SIZE = (1 << 10) - 1;

struct Page {
    std::array<PriceLevel, 1024> levels;
};
class OrderBook {
private:
        OrderPool order_pool;                      
        std::vector<Page*> directory;        // directory for memory pages
        std::unordered_map<uint64_t, order_tracker> order_map;            // O(1) map for cancellations
        uint32_t best_bid = 0;
        uint32_t best_ask = 0xFFFFFFFF;
        lockfreeRingbuffer<TradeDetails, 65536> trade_queue;           //instantiate the ringbuffer
    // function to return page and slot
        PriceLevel* getPriceLevel(uint32_t price) {
            uint32_t page_index = price >> PAGE_SHIFT;
            uint32_t slot_index = price & SLOT_SIZE;

            // allocation in case of memory outflow
            if (page_index >= directory.size()) {
                directory.resize(page_index + 100, nullptr);    // if price>4000000
            }
            if (directory[page_index] == nullptr) {
                directory[page_index] = new Page();                 // if new price page is unlocked
            }

            return &directory[page_index]->levels[slot_index];
        }
        void wait_for_buffer(const TradeDetails& trade) {
            while (!trade_queue.push(trade)) {
                _mm_pause();
            }
        }
public:
    explicit OrderBook(uint32_t capacity = 1000000) : order_pool(capacity) {
        // Pre-allocate pointers for prices up to 40 million
        directory.resize(40000, nullptr);
    }
    ~OrderBook() {
        for (Page* p : directory) {
            delete p;
        }
    }
    
    void addOrder(uint64_t id, uint32_t price, uint32_t qty, Side side, OrderType type, TimeInForce tif) {
        uint32_t remaining_qty = qty;


        if (side == Side::Bid) {
            // Aggressive Market/Limit order crossing the spread
            while (remaining_qty > 0 && best_ask <= price) {
                    PriceLevel* ask_level = getPriceLevel(best_ask);

                if (ask_level->isEmpty()) {
                    best_ask++; // Walk the book up
                    continue;
                }

                uint32_t available = ask_level->getVolume();
                TradeDetails trade{id, 0, best_ask, 0, 0};
                if (available <= remaining_qty) {    // if the shares available are less than we wanted

                    trade.quantity = available;
                    wait_for_buffer(trade);
                    remaining_qty -= available;       // calculates remaining shares to buy
                    best_ask++;                       // gets to next best seller
                } else {
                    trade.quantity = remaining_qty;
                    wait_for_buffer(trade);
                    // normal case when full required quantity is bought
                    ask_level->decreaseVolume(remaining_qty);
                    remaining_qty = 0;
                }
            }
        } else {
            while (remaining_qty > 0 && best_bid >= price) {
                PriceLevel* bid_level = getPriceLevel(best_bid);

                if (bid_level->isEmpty()) {
                    if (best_bid == 0) break;           // gets out of loop if book is empty
                    best_bid--;                         // gets to next highest price
                    continue;
                }

                uint32_t available = bid_level->getVolume();
                TradeDetails trade{0, id, best_bid, 0, 0};
                if (available <= remaining_qty) {
                    trade.quantity = available;
                    wait_for_buffer(trade);
                    // if buyer wants less shares than we have to sell

                    remaining_qty -= available;         // calculates remaining shares to sell
                    best_bid--;                         // gets to next best buyer
                } else {
                    trade.quantity = remaining_qty;
                    wait_for_buffer(trade);
                    // normal case when full required quantity is sold
                    bid_level->decreaseVolume(remaining_qty);
                    remaining_qty = 0;
                }
            }
        }

        // --- 2. TIME-IN-FORCE ROUTING ---
        if (remaining_qty == 0) return; // Order was fully executed, nothing to rest

        if (tif == TimeInForce::IOC || tif == TimeInForce::FOK) {
            // The algorithm refuses to rest in the queue. Send a cancel confirm.
            std::cout << "IOC Triggered: " << remaining_qty << " shares of Order " << id << " cancelled." << std::endl;
            return;
        }

        // --- 3. RESTING ORDER PLACEMENT (O(1)) ---
        // Pop a raw memory index from the Free List stack
        uint32_t pool_idx = order_pool.Allocate_Order_Memory(id, price, remaining_qty, type, side, tif);

        // Find exact memory location using bitwise routing
        PriceLevel* level = getPriceLevel(price);
        level->addOrder(pool_idx, order_pool);

        // Save the direct pointer for instant cancellation later
        order_map[id] = {pool_idx, level};

        // Update Top of Book Trackers
        if (side == Side::Bid && price > best_bid) best_bid = price;
        if (side == Side::Ask && price < best_ask) best_ask = price;
    }

    void cancelOrder(uint64_t id) {
        // 1. Instant Lookup
        auto it = order_map.find(id);
        if (it == order_map.end()) return;

        order_tracker tracker = it->second;

        // 2. Pure O(1) Extraction (Zero Tree Traversals!)
        tracker.level->removeOrder(tracker.pool_index, order_pool);

        // 3. Return memory to Free List
        order_pool.Deallocate_Order_Memory(tracker.pool_index);
        order_map.erase(it);

        // Note: If this cancel emptied out the Best Bid/Ask, the next incoming
        // trade will automatically "walk" the BBO pointer to the correct price.
    }
};


