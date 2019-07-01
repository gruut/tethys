cmake_minimum_required(VERSION 3.10)

file(GLOB HEADERS
        "include/*.hpp"
        )

file(GLOB SOURCES
        "*.cpp"
        )

add_library(block_producer_plugin
        ${HEADERS}
        ${SOURCES})

find_package(Soci)
if (SOCI_FOUND)
    target_link_libraries(block_producer_plugin ${SOCI_LIBRARY} ${SOCI_mysql_PLUGIN})
else ()
    message(WARNING "SOCI NOT FOUND")
endif ()

target_link_libraries(block_producer_plugin appbase log)
target_include_directories(block_producer_plugin PUBLIC
        "/usr/local/include/mysql"
        "/usr/include/mysql"
        "${SOCI_INCLUDE_DIR}"

        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        )
