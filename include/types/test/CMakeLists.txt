cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

file(GLOB HEADER_FILES
        "../transaction.hpp"
        )
find_package(RTTR CONFIG REQUIRED Core)

add_executable(transaction_test transaction_test.cpp)

target_include_directories(transaction_test PUBLIC /usr/local/include)
target_link_libraries(transaction_test RTTR::Core)

target_sources(transaction_test
        PUBLIC
        ${HEADER_FILES}
        )
