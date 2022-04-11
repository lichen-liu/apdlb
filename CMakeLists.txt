cmake_minimum_required(VERSION 3.10.0)
project(apert)
include(CTest)

add_subdirectory(src/ert ert)

if(DEFINED ENV{ROSE_PATH})
    add_subdirectory(src/ap ap)
    message(STATUS "ap is enabled since ROSE Compiler is found")
else()
    message(WARNING "ROSE Compiler is not found! Skipping ap")
endif()

add_subdirectory(benchmark)
add_subdirectory(apert_gen)