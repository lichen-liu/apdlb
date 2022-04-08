cmake_minimum_required(VERSION 3.7.0)
project(kernels)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3 -std=c++17)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# CMAKE_SOURCE_DIR
include_directories(tp)

add_executable(collatz_conjecture_bm collatz_conjecture_bm.cpp)
add_test(kernels_collatz_conjecture_bm collatz_conjecture_bm)

add_executable(matvecp_bm matvecp_bm.cpp)
add_test(kernels_matvecp_bm matvecp_bm)

add_executable(sorting_bm sorting_bm.cpp)
add_test(kernels_sorting_bm sorting_bm)

# Add test executable here
add_custom_target(kernels_bm DEPENDS
    collatz_conjecture_bm
    matvecp_bm
    sorting_bm)