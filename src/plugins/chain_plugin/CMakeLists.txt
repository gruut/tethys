cmake_minimum_required(VERSION 3.10)

add_library(chain_plugin
        chain_plugin.cpp
        chain.cpp
        include/chain_plugin.hpp)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

find_package(Soci)
if(SOCI_FOUND)
    target_link_libraries(chain_plugin ${SOCI_LIBRARY} ${SOCI_mysql_PLUGIN})
else()
    message(WARNING "SOCI NOT FOUND")
endif()

find_package(RTTR CONFIG REQUIRED Core)
if(RTTR_FOUND)
    target_link_libraries(chain_plugin RTTR::Core)
else()
    message(WARNING "RTTR NOT FOUND")
endif()

target_link_libraries(chain_plugin appbase log)
target_include_directories(chain_plugin PUBLIC
        "/usr/local/include/mysql"
        "/usr/include/mysql"
        "${SOCI_INCLUDE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        )
