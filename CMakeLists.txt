cmake_minimum_required(VERSION 3.10.0)
project(apdlb)

include(CTest)

add_subdirectory(src/tp tp)

if(DEFINED ENV{ROSE_PATH})
    add_subdirectory(src/rc rc)
    message(STATUS "rc is enabled since ROSE Compiler is found")
else()
    message(WARNING "ROSE Compiler is not found! Skipping rc")
endif()