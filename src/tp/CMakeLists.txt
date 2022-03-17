cmake_minimum_required(VERSION 3.0.0)
project(cpusim VERSION 0.1.0)

find_package(Threads REQUIRED)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE tp_lib_SRC
    "*.cpp"
)
add_library(tp ${tp_lib_SRC})
target_link_libraries(tp PRIVATE Threads::Threads)