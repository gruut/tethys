cmake_minimum_required(VERSION 3.10)

file(GLOB HEADERS
        "include/*.hpp"
        "kademlia/include/*.hpp"
        "rpc_services/include/*.hpp"
        )

file(GLOB SOURCES
        "*.cpp"
        "kademlia/*.cpp"
        "rpc_services/*.cpp"
        )

add_library(net_plugin
        ${HEADERS}
        ${SOURCES})

target_link_libraries(net_plugin appbase log)
target_include_directories(net_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        )
