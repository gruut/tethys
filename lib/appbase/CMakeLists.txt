project(appbase)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BOOST_COMPONENTS)
list(APPEND BOOST_COMPONENTS
        filesystem
        program_options
        )
set(Boost_USE_MULTITHREADED OFF)
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

file(GLOB HEADERS "*.hpp")

add_library(appbase
        application.cpp
        ${HEADERS}
        )

target_link_libraries(appbase ${Boost_LIBRARIES})

add_subdirectory(examples)
