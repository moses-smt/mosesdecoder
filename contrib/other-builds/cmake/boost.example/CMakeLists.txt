cmake_minimum_required(VERSION 2.8)

find_package(Boost 1.36.0 REQUIRED COMPONENTS locale)

include_directories(${Boost_INCLUDE_DIRS})
add_executable(demo main.cpp)
target_link_libraries(demo ${Boost_LIBRARIES})
