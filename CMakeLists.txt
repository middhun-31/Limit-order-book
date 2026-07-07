cmake_minimum_required(VERSION 3.20)                        #setting cmake version to 3.20
project(LOB_ENGINE VERSION 1.0 LANGUAGES CXX)               #setting project name and version

#enforcing cpp 20 standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#setting up guardrails to optimise for performance and better compiler warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O3 -march=native")

#including all header files
include_directories(include)

#directing cmake towards the main executable
add_executable(MatchingEngine src/main.cpp
        src/main.cpp
        include/order.h
        include/PriceLevel.h
        include/OrderPool.h
        include/OrderBook.h
        include/RingBuffer.h
        )