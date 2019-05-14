cmake_minimum_required(VERSION 3.10)

find_package(Boost REQUIRED COMPONENTS system thread random)

file(GLOB HEADERS
        "include/*.hpp"
        "rpc_services/proto/include/*.h"
        "rpc_services/include/*.hpp"
        )

file(GLOB SOURCES
        "rpc_services/proto/*.cc"
        "rpc_services/*.cpp"
        "*.cpp"
        )

set(LIB_PREFIX "/usr/local/lib")

# TODO : write `FindGrpc.cmake`

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

add_library(admin_plugin
        ${HEADERS}
        ${SOURCES})

target_link_libraries(admin_plugin
        appbase
        log
        json
        ${Boost_LIBRARIES}
        ${PROTOBUF_LIB}
        ${GRPC_LIBS}
        )

target_include_directories(admin_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        "${CMAKE_CURRNET_SOURCE_DIR}/../../../lib/json"
        "${GRUUT_INSTALL_DIR}/include"
        ${Boost_INCLUDE_DIR}
        )
