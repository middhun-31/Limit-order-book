#pragma once
#include "order.h"
#include <vector>
#include <stdexcept>

class OrderPool {
private:
    std::vector<order> pool;           // to pre-allocate order pool
    std::vector<uint32_t> free_list;   // to track list of free memory
public:
    explicit OrderPool(uint32_t capacity) {
        pool.resize(capacity);                  //allocates the whole capacity i.e. 10 million orders with default values
        free_list.reserve(capacity);            // stores integers which denote which memory location is free
        for (uint32_t i = capacity; i > 0; --i) {       // loops through free_list and stores index in reverse order i.e 0 is on top and largest index at bottom
            free_list.push_back(i-1);
        }
    }
    uint32_t Allocate_Order_Memory(uint64_t id, uint64_t price, uint32_t volume, OrderType order_type, Side side, TimeInForce tf) {               // function to allocate memory for each order
        if (free_list.empty()) {
            throw std::runtime_error("Memory pool exhausted");                                                              //throws runtime error if memory pool is exhausted
        }

        uint32_t index = free_list.back();                                               // checks the free index in free_list
        free_list.pop_back();                                                            // takes the index of free_list
        pool[index].reset(id, price, volume, side, order_type, tf);                          // calls reset() in order.h to allocate memory and initialise parameters for order in the memory index returned by free_list
        return index;
    }
    void Deallocate_Order_Memory(uint32_t index) {
        free_list.push_back(index);                                                     // pushes back deallocated index to free_list
    }
    order& get_order(uint32_t index) {
        return pool[index];                                                            // function to search order by index
    }
};