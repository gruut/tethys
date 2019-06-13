cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BOOST_COMPONENTS)

set(BOTAN_LIBS "${LIB_PREFIX}/libbotan-2.a")

list(APPEND BOOST_COMPONENTS
        unit_test_framework
        system
        thread
        filesystem
        )
find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

set(LOGGER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/log")
set(TINYXML_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/tinyxml")

file(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.hpp")
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../*.cpp")

add_executable(comp_test ${HEADERS} ${SOURCES} component_test.cpp)
target_include_directories(comp_test PUBLIC "${TINYXML_DIRS}/include ${LOGGER_DIRS}/include")
target_link_libraries(comp_test
        PRIVATE
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_LIBRARIES}
        ${BOTAN_LIBS}

        log
        tinyxml
        )
