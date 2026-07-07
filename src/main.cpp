#include "OrderBook.h"
#include <iostream>

int main() {
    std::cout << "=========================================\n";
    std::cout << "   LOW-LATENCY MATCHING ENGINE TEST      \n";
    std::cout << "=========================================\n\n";

    // Initialize the engine with a pool capacity of 10,000 orders
    OrderBook book(10000);

    // ---------------------------------------------------------
    // SCENARIO 1: BUILD THE INITIAL ORDER BOOK (PASSIVE ORDERS)
    // ---------------------------------------------------------
    std::cout << "[1] Populating Book with Passive Orders...\n";

    // Sellers (Asks)
    book.addOrder(101, 150, 200, Side::Ask, OrderType::LIMIT, TimeInForce::GTC); // Seller wants $150
    book.addOrder(102, 151, 500, Side::Ask, OrderType::LIMIT, TimeInForce::GTC); // Seller wants $151

    // Buyers (Bids)
    book.addOrder(201, 148, 100, Side::Bid, OrderType::LIMIT, TimeInForce::GTC); // Buyer paying $148
    book.addOrder(202, 147, 300, Side::Bid, OrderType::LIMIT, TimeInForce::GTC); // Buyer paying $147

    std::cout << "    -> Book built. Spread is $148 (Bid) to $150 (Ask).\n\n";

    // ---------------------------------------------------------
    // SCENARIO 2: AGGRESSIVE BUYER CROSSES THE SPREAD
    // ---------------------------------------------------------
    std::cout << "[2] Incoming Aggressive Buyer (Order 301): Wants 250 shares at $151...\n";
    // This order should wipe out the 200 shares at $150, and take 50 shares from the $151 level.
    book.addOrder(301, 151, 250, Side::Bid, OrderType::LIMIT, TimeInForce::GTC);

    // ---------------------------------------------------------
    // SCENARIO 3: PROCESS THE LOCK-FREE QUEUE
    // ---------------------------------------------------------
    std::cout << "\n[3] Network Thread reading from Lock-Free Ring Buffer:\n";
    TradeDetails trade;
    int trades_processed = 0;

    // In a real system, a background thread runs this while loop infinitely.
    // For our test, we just poll until it returns false (empty).
    while (book.pollTrade(trade)) {
        std::cout << "    >>> TRADE EXECUTED: "
                  << "Buyer ID " << trade.buyer_order_id << " bought "
                  << trade.quantity << " shares "
                  << "at Price $" << trade.price << "\n";
        trades_processed++;
    }

    if (trades_processed == 0) {
        std::cout << "    [!] Error: No trades were generated!\n";
    }

    // ---------------------------------------------------------
    // SCENARIO 4: IMMEDIATE-OR-CANCEL (IOC) BEHAVIOR
    // ---------------------------------------------------------
    std::cout << "\n[4] Testing IOC logic: Incoming Buyer (Order 401) wants 1000 shares at $151...\n";
    // There are only 450 shares left at $151. It should buy those, and instantly cancel the remaining 550.
    book.addOrder(401, 151, 1000, Side::Bid, OrderType::LIMIT, TimeInForce::IOC);

    std::cout << "\n[5] Network Thread reading final trades:\n";
    while (book.pollTrade(trade)) {
        std::cout << "    >>> TRADE EXECUTED: "
                  << "Buyer ID " << trade.buyer_order_id << " bought "
                  << trade.quantity << " shares "
                  << "at Price $" << trade.price << "\n";
    }

    std::cout << "\nTest Complete.\n";
    return 0;
}