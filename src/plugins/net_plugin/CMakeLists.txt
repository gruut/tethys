cmake_minimum_required(VERSION 3.10)

find_package(Boost REQUIRED COMPONENTS system thread random)
find_package(CURL REQUIRED)

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
set(PROTOBUF_LIB "${LIB_PREFIX}/libprotobuf.a")

if (APPLE)
    set(GRPC_LIBS
            "${LIB_PREFIX}/libgrpc++.dylib"
            "${LIB_PREFIX}/libgrpc.dylib"
            "${LIB_PREFIX}/libgrpc++_cronet.dylib"
            "${LIB_PREFIX}/libgrpc++_error_details.dylib"
            "${LIB_PREFIX}/libgrpc++_reflection.dylib"
            "${LIB_PREFIX}/libgrpc++_unsecure.dylib"
            "${LIB_PREFIX}/libgrpcpp_channelz.dylib")
else ()
    set(GRPC_LIBS
            "${LIB_PREFIX}/libgrpc++.so"
            "${LIB_PREFIX}/libgrpc.so"
            "${LIB_PREFIX}/libgrpc++_cronet.so"
            "${LIB_PREFIX}/libgrpc++_error_details.so"
            "${LIB_PREFIX}/libgrpc++_reflection.so"
            "${LIB_PREFIX}/libgrpc++_unsecure.so"
            "${LIB_PREFIX}/libgrpcpp_channelz.so")
endif ()

add_library(net_plugin
        ${HEADERS}
        ${SOURCES})

target_link_libraries(net_plugin
        appbase
        log
        ${CURL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${BOTAN_LIB}
        ${PROTOBUF_LIB}
        ${GRPC_LIBS}
        )
target_include_directories(net_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        ${Boost_INCLUDE_DIR}
        ${CURL_INCLUDE_DIR}
        )
