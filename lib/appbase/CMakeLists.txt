project(appbase)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB HEADERS "*.hpp")

add_library(appbase
        application.cpp
        ${HEADERS}
        )
