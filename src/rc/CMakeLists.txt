cmake_minimum_required(VERSION 3.0.0)
project(rc VERSION 0.1.0)
include(CTest)

if(DEFINED ENV{ROSE_PATH})
    set(ROSE_INSTALL_DIR $ENV{ROSE_PATH})
else()
    message(FATAL_ERROR "ROSE_PATH env is not defined for rc")
endif()

add_compile_options(-Werror -Wall -Wno-misleading-indentation -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set up ROSE
set(ROSE_INCLUDE_DIR ${ROSE_INSTALL_DIR}/include)
include_directories(${ROSE_INCLUDE_DIR} ${ROSE_INCLUDE_DIR}/rose)
message(STATUS "ROSE include = ${ROSE_INCLUDE_DIR}")

set(ROSE_LIB_DIR ${ROSE_INSTALL_DIR}/lib)
link_directories(${ROSE_LIB_DIR})
message(STATUS "ROSE lib = ${ROSE_LIB_DIR}")

# Set up Boost
find_package(Boost COMPONENTS system filesystem REQUIRED)

file(GLOB_RECURSE rc_lib_SRC
    "*.cpp"
)
add_library(rc ${rc_lib_SRC})
target_link_libraries(rc librose.so ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

add_executable(rc_exe main.cc)
target_link_libraries(rc_exe rc)

add_executable(autoPar autoPar.cc)
target_link_libraries(autoPar rc)

add_custom_target(rc_all DEPENDS
    rc
    rc_exe
    autoPar)