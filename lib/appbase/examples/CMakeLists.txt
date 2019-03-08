cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

add_executable(appbase_example main.cpp)
target_link_libraries(appbase_example appbase)
