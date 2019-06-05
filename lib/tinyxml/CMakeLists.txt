cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(InstallDirectoryPermissions)

file(GLOB HEADERS "include/*.h")

add_library(tinyxml
        tinyxml2.cpp
        ${HEADERS}
        )

set_target_properties(tinyxml PROPERTIES PUBLIC_HEADER "${HEADERS}")
