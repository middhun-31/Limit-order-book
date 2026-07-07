#pragma once
#include<cstdint>
//choice of trader to sell or buy
enum class Side : uint8_t{
    Bid, Ask
};
//weather trader wants a cap on price or volume of shares
enum class OrderType: uint8_t {
    LIMIT, MARKET
};

enum class TimeInForce : uint8_t {
    GTC,                             //Good-till-Cancelled: rests the unfulfilled order in queue until cancelled
    IOC,                             //Immediate-or-cancel: get whatever is available and drop the rest
    FOK                              //Fill-or-Kill: either process the full order or drop everything
};
constexpr uint32_t NULL_INDEX = 0xFFFFFFFF;
//data structure of an order
struct order {
    uint64_t order_id;                                // unique id for each order
    uint64_t price;                                   // price of share stored in lowest subunit
    uint32_t volume;                                  // number of shares
    Side side;
    OrderType order_type;
    TimeInForce timeInForce;
    uint32_t prev_index = NULL_INDEX;
    uint32_t next_index = NULL_INDEX;
    order() = default;                               // default constructor

    //initializing values with parametrized constructor
    void reset(uint64_t id , uint64_t p , uint32_t v, Side s , OrderType t , TimeInForce tf) {
        order_id=id;
        price=p;
        volume=v;
        side=s;
        order_type=t;
        timeInForce=tf;
        prev_index = NULL_INDEX;
        next_index = NULL_INDEX;
    };

};