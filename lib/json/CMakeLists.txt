cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")
include(InstallDirectoryPermissions)

file(GLOB HEADERS "include/*.hpp")
file(GLOB SOURCES "*.cpp")

add_library(json
        ${HEADERS}
        ${SOURCES}
        )

set_target_properties(json PROPERTIES PUBLIC_HEADER "${HEADERS}" LINKER_LANGUAGE CXX)

install(TARGETS
        json

        RUNTIME DESTINATION ${GRUUT_INSTALL_DIR}/bin
        LIBRARY DESTINATION ${GRUUT_INSTALL_DIR}/lib
        ARCHIVE DESTINATION ${GRUUT_INSTALL_DIR}/lib
        PUBLIC_HEADER DESTINATION ${GRUUT_INSTALL_DIR}/include/json
        )
install_directory_permissions(DIRECTORY ${GRUUT_INSTALL_DIR})

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_subdirectory(test)
endif()
