cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

file(GLOB HEADERS
        *.hpp
        include/*.hpp
        config/*.hpp
        structure/*.hpp
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tethys-utils/src/ags.hpp"
        )
file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tethys-utils/src/ags.cpp"
        *.cpp
        )
add_library(chain_plugin
        ${HEADERS}
        ${SOURCES}
        )

find_package(Boost REQUIRED COMPONENTS system filesystem)

find_package(LevelDB)
if (LEVELDB_FOUND)
    target_link_libraries(chain_plugin ${LevelDB_LIBRARY})
endif (LEVELDB_FOUND)

find_package(Mysql)
if (MYSQL_FOUND)
    target_link_libraries(chain_plugin ${MYSQL_LIBRARIES})
else (MYSQL_FOUND)
    message(WARNING "MYSQL NOT FOUND")
endif (MYSQL_FOUND)

find_package(Soci)
if (SOCI_FOUND)
    target_link_libraries(chain_plugin ${SOCI_LIBRARY} ${SOCI_mysql_PLUGIN})
else (SOCI_FOUND)
    message(WARNING "SOCI NOT FOUND")
endif (SOCI_FOUND)

set(LIB_PREFIX "/usr/local/lib")
set(BOTAN_LIB "${LIB_PREFIX}/libbotan-2.a")

target_link_libraries(chain_plugin appbase log tinyxml
        ${Boost_LIBRARIES}
        ${BOTAN_LIB}
        )

target_include_directories(
        chain_plugin
        PUBLIC
        "/usr/local/include/mysql"
        "/usr/include/mysql"
        "${SOCI_INCLUDE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/appbase"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/log"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tethys-utils/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/tinyxml"
        )
