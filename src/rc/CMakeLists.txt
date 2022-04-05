cmake_minimum_required(VERSION 3.0.0)
project(rc VERSION 0.1.0)
include(CTest)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE rc_lib_SRC
    "*.cpp"
)
add_library(rc ${rc_lib_SRC})
target_link_libraries(rc)

add_executable(rc_exe main.cc)
target_link_libraries(rc_exe rc)

add_custom_target(rc_all)
add_dependencies(rc_all rc rc_exe)