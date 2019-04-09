cmake_minimum_required(VERSION 3.10)

file(GLOB HEADERS
        "include/*.hpp"
        )

file(GLOB SOURCES
        "*.cpp"
        )

set(LIB_PREFIX "/usr/local/lib")
set(BOTAN_LIB "${LIB_PREFIX}/libbotan-2.a")

add_library(signer_manager_plugin
        ${HEADERS}
        ${SOURCES})

target_link_libraries(signer_manager_plugin
        appbase
        log
        json
        ${BOTAN_LIB}
        )

target_include_directories(signer_manager_plugin PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/gruut-utils/src"
        "${CMAKE_CURRNET_SOURCE_DIR}/../../../lib/json"
        "${GRUUT_INSTALL_DIR}/include"
        )
