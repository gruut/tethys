cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

if(NOT DEFINED ${CMAKE_INSTALL_PREFIX})
    set(CMAKE_INSTALL_PREFIX "/usr/local")
    set(GRUUT_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/gruut)
else()
    set(GRUUT_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
endif()

project(gruut LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")
include(InstallDirectoryPermissions)

add_executable(
        gruut
        src/main.cpp
)

add_subdirectory(lib)
add_subdirectory(src/plugins)

target_link_libraries(gruut
        appbase
        log
        net_plugin
        block_producer_plugin
        chain_plugin
        signer_manager_plugin
        )

target_include_directories(gruut
        PRIVATE
        "${CMAKE_INSTALL_PREFIX}/include"
        )

install(DIRECTORY include/ DESTINATION ${GRUUT_INSTALL_DIR}/include)
install_directory_permissions(DIRECTORY ${GRUUT_INSTALL_DIR}/include)

IF (NOT $ENV{TRAVIS})
    add_custom_command(
            TARGET gruut_utils
            POST_BUILD
            COMMAND clang-format -i ./src/*.hpp
            COMMAND clang-format -i ./src/*.cpp
    )
ENDIF ()
