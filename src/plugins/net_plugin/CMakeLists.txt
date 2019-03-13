cmake_minimum_required(VERSION 3.10)

add_library(net_plugin
        net_plugin.cpp
        include/net_plugin.hpp)

target_link_libraries(net_plugin appbase log)
target_include_directories(net_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        )
