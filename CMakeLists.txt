cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

if(NOT DEFINED ${CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "/usr/local")
    set(TETHYS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/tethys)
else()
    set(TETHYS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
endif()

project(tethys LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")
include(InstallDirectoryPermissions)

add_executable(
        tethys
        src/main.cpp
)

add_subdirectory(lib)
add_subdirectory(src/plugins)
add_subdirectory(src/contract)

target_link_libraries(tethys
        appbase
        log
        admin_plugin
        net_plugin
        block_producer_plugin
        chain_plugin
        )

target_include_directories(tethys
        PUBLIC
        "${CMAKE_INSTALL_PREFIX}/include"
        )

install(DIRECTORY include/ DESTINATION ${TETHYS_INSTALL_DIR}/include)
install_directory_permissions(DIRECTORY ${TETHYS_INSTALL_DIR}/include)

IF (NOT $ENV{TRAVIS})
    add_custom_command(
            TARGET tethys_utils
            POST_BUILD
            COMMAND clang-format -i ./src/*.hpp
            COMMAND clang-format -i ./src/*.cpp
    )
ENDIF ()
