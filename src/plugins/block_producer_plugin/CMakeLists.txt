cmake_minimum_required(VERSION 3.10)

add_library(block_producer_plugin
        block_producer_plugin.cpp
        include/block_producer_plugin.hpp)

target_link_libraries(block_producer_plugin appbase)
target_include_directories(block_producer_plugin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase")
