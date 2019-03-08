project(log)

cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB HEADERS "include/*.hpp")

add_library(log
        log.cpp
        ${HEADERS}
        )

set_target_properties(log PROPERTIES PUBLIC_HEADER "${HEADERS}")
target_include_directories(log PRIVATE "${PROJECT_SOURCE_DIR}/include/spdlog")

install(TARGETS
        log

        RUNTIME DESTINATION ${GRUUT_INSTALL_DIR}/bin
        LIBRARY DESTINATION ${GRUUT_INSTALL_DIR}/lib
        ARCHIVE DESTINATION ${GRUUT_INSTALL_DIR}/lib
        PUBLIC_HEADER DESTINATION ${GRUUT_INSTALL_DIR}/include/log
        )
