cmake_minimum_required(VERSION 3.10)

find_package(Boost REQUIRED COMPONENTS system thread random)
find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)

file(GLOB HEADERS
        "include/*.hpp"
        "kademlia/include/*.hpp"
        "rpc_services/include/*.hpp"
        "rpc_services/protos/include/*.h"
        "config/include/*.hpp"
        )

file(GLOB SOURCES
        "*.cpp"
        "kademlia/*.cpp"
        "rpc_services/*.cpp"
        "rpc_services/protos/*.cc"
        )

set(LIB_PREFIX "/usr/local/lib")
set(BOTAN_LIB "${LIB_PREFIX}/libbotan-2.a")
set(LZ4_LIB "${LIB_PREFIX}/liblz4.a")

add_library(net_plugin
        ${HEADERS}
        ${SOURCES})

target_link_libraries(net_plugin
        appbase
        log
        json
        ${BOTAN_LIB}
        ${LZ4_LIB}
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}

        ${GRPC_GRPC++_LIBRARY}
        ${GRPC_LIBRARY}
        ${GRPC_GRPC++_REFLECTION_LIBRARY}
        )

target_include_directories(net_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tethys-utils/src"
        "${CMAKE_CURRNET_SOURCE_DIR}/../../../lib/json"
        "${GRUUT_INSTALL_DIR}/include"
        )
