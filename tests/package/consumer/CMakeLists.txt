cmake_minimum_required(VERSION 3.24)
project(mmat_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(mmat 0.1 CONFIG REQUIRED)

add_executable(mmat_consumer main.cpp)
target_link_libraries(mmat_consumer PRIVATE mmat::mmat)
