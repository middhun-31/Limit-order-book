#pragma once
#include "order.h"
#include "OrderPool.h"
#include <cstdint>

class PriceLevel {
private:
    uint64_t price;
    uint32_t total_volume = 0;
    uint32_t head_index = NULL_INDEX;
    uint32_t tail_index = NULL_INDEX;

public:
    PriceLevel() = default;

    // get functions to access private members
    [[nodiscard]] uint64_t getPrice() const { return price; }
    [[nodiscard]] uint32_t getVolume() const { return total_volume; }
    [[nodiscard]] bool isEmpty() const { return head_index== NULL_INDEX; }
    [[nodiscard]] uint32_t getHead() const { return head_index; }


    void addOrder(uint32_t order_index, OrderPool& pool) {
        order& order = pool.get_order(order_index);

        order.next_index = NULL_INDEX;
        order.prev_index = tail_index;

        if (tail_index != NULL_INDEX) {
            pool.get_order(tail_index).next_index = order_index; // Link previous last order to this one
        } else {
            head_index = order_index; // If list was empty, this is now the head
        }

        tail_index = order_index; // The new order is now the tail
        total_volume += order.volume;
    }

    // O(1) Removal using array indices
    void removeOrder(uint32_t order_idx, OrderPool& pool) {
        order& order = pool.get_order(order_idx);
        total_volume -= order.volume;

        // Re-link previous node
        if (order.prev_index != NULL_INDEX) {
            pool.get_order(order.prev_index).next_index = order.next_index;
        } else {
            head_index = order.next_index; // We are removing the head
        }

        // Re-link next node
        if (order.next_index != NULL_INDEX) {
            pool.get_order(order.next_index).prev_index = order.prev_index;
        } else {
            tail_index = order.prev_index; // We are removing the tail
        }

        // Wipe indices for safety before this index gets recycled by the Free List
        order.prev_index = NULL_INDEX;
        order.next_index = NULL_INDEX;
    }

    void decreaseVolume(uint32_t fill_qty) {
        total_volume -= fill_qty;
    }
};