project(appbase)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules")

include(InstallDirectoryPermissions)

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

file(GLOB HEADERS "include/*.hpp")

add_library(appbase
        application.cpp
        ${HEADERS}
        )

target_link_libraries(appbase ${Boost_LIBRARIES})

set_target_properties(appbase PROPERTIES PUBLIC_HEADER "${HEADERS}")

install(TARGETS
        appbase

        RUNTIME DESTINATION ${GRUUT_INSTALL_DIR}/bin
        LIBRARY DESTINATION ${GRUUT_INSTALL_DIR}/lib
        ARCHIVE DESTINATION ${GRUUT_INSTALL_DIR}/lib
        PUBLIC_HEADER DESTINATION ${GRUUT_INSTALL_DIR}/include/appbase
        )
install_directory_permissions(DIRECTORY ${GRUUT_INSTALL_DIR})

add_subdirectory(examples)
