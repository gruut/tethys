cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(gruut LANGUAGES CXX)

add_executable(
        gruut
        src/main.cpp
)

add_subdirectory(lib)
add_subdirectory(src/plugins)

target_link_libraries(gruut
        appbase
        )

IF (NOT $ENV{TRAVIS})
    add_custom_command(
            TARGET gruut_utils
            POST_BUILD
            COMMAND clang-format -i ./src/*.hpp
            COMMAND clang-format -i ./src/*.cpp
    )
ENDIF ()

install()