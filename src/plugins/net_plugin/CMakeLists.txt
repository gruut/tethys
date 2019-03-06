cmake_minimum_required(VERSION 3.10)

add_library(net_plugin
        net_plugin.cpp
        net_plugin.hpp)

target_link_libraries(net_plugin appbase)
target_include_directories(net_plugin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase")
