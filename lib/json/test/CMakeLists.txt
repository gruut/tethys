cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

file(GLOB HEADER_FILES
        "../include/json.hpp"
        )

add_executable(json_test json_test.cpp)

target_sources(json_test
        PUBLIC
        ${HEADER_FILES}
        )

target_link_libraries(json_test
        PRIVATE
        )
