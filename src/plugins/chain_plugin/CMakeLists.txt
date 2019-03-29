cmake_minimum_required(VERSION 3.10)

add_library(chain_plugin
        chain_plugin.cpp
        include/chain_plugin.hpp)

target_link_libraries(chain_plugin appbase log)
target_include_directories(chain_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        )
