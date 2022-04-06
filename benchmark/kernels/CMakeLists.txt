cmake_minimum_required(VERSION 3.7.0)
project(kernels)

find_package(Threads REQUIRED)

# CMAKE_SOURCE_DIR
include_directories(tp)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

link_libraries(Threads::Threads)

add_executable(collatz_conjecture_bm collatz_conjecture_bm.cpp)
add_test(kernels_collatz_conjecture_bm collatz_conjecture_bm)

# Add test executable here
add_custom_target(kernels_bm DEPENDS
    collatz_conjecture_bm)
