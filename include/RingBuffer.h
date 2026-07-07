#pragma once
#include <atomic>
#include <cstdint>
#include <new>

struct TradeDetails {                   // the container which carries information of trade
    uint64_t buyer_order_id;
    uint64_t seller_order_id;
    uint32_t price;
    uint32_t quantity;
    uint64_t timestamp;
};

constexpr std::size_t CACHE_LINE_SIZE = std::hardware_destructive_interference_size;       //to prevent false sharing

template<typename T, size_t capacity>
class lockfreeRingbuffer {
private:
    static_assert((capacity & (capacity-1)) == 0, "capacity must be a power of two");    //making sure capacity is a power of 2
    static constexpr size_t MASK = capacity-1;                                           //so that we can use bitwise manipulation on capacity
                                                                                         //instead of modulo operator
    T buffer[capacity];

    alignas(CACHE_LINE_SIZE) std::atomic<size_t> write_index{0};                      //makes sure write and read operations
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> read_index{0};                       //use completely different cache lines

public:
    lockfreeRingbuffer() = default;

    bool push(const T& item) {
        const size_t current_write = write_index.load(std::memory_order_relaxed);        //gets the current write index; memery_order_relaxed as only producer queue is accessing it
        const size_t current_read = read_index.load(std::memory_order_acquire);          //gets the current read index; memery_order_acquire as many consumers read and needs syncronisation
        if (current_write - current_read >= capacity) {
            return false;                      // condition for buffer full
        }

        buffer[current_write & MASK] = item;              //bit manipulation to reuse ring buffer without resetting order no
        write_index.store(current_write + 1, std::memory_order_release);               //releases newly updated write index to consumers
        return true;
    }

    bool pop(T& item) {
        const size_t current_read = read_index.load(std::memory_order_relaxed);
        const size_t current_write = write_index.load(std::memory_order_acquire);
        if (current_write == current_read) {
            return false;                      // queue is empty
        }

        item = buffer[current_read & MASK];        //reads data for consumer to send out to outside world
        read_index.store(current_read + 1, std::memory_order_release);
        return true;
    }



};
