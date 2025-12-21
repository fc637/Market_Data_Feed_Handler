cmake_minimum_required(VERSION 3.10)
project(ExchangeSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# Include directories
# =========================
include_directories(
    src/common
    src/server
    src/client
)

# =========================
# Common sources (shared)
# =========================
add_executable(exchange_simulator
    src/server/exchange_simulator.cpp
    src/server/server.cpp
    src/server/client_manager.cpp
    src/server/tick_generator.cpp
    src/common/cache.cpp
    src/common/latency_tracker.cpp
    src/common/memory_pool.cpp
)

# =========================
# Feed Handler (Client + UI)
# =========================
add_executable(feed_handler
    src/client/feed_handler.cpp
    src/client/socket.cpp
    src/client/parser.cpp
    src/client/visualizer.cpp
    src/common/cache.cpp
    src/common/latency_tracker.cpp
    src/common/memory_pool.cpp
)

# =========================
# Platform-specific libs
# =========================
if(UNIX)
    target_link_libraries(feed_handler pthread)
endif()
