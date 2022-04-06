cmake_minimum_required(VERSION 3.0.0)
project(tp VERSION 0.1.0)
include(CTest)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(tests)