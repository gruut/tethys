cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIB_PREFIX "/usr/local/lib")
set(BOTAN_LIBS "${LIB_PREFIX}/libbotan-2.a")

find_package(Boost COMPONENTS unit_test_framework system thread filesystem REQUIRED)
IF (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIR})
ENDIF (Boost_FOUND)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

file(GLOB HEADERS
        "include/*.hpp"
    )

add_library(contract
        ${HEADERS})

set_target_properties(contract PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(contract
        ${Boost_LIBRARIES}
        ${BOTAN_LIBS}
        )

target_include_directories(contract PUBLIC "${GRUUT_INSTALL_DIR}/include")
