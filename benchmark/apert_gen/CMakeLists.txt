cmake_minimum_required(VERSION 3.7.0)
project(apert_gen)
include(CTest)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3 -std=c++17)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# CMAKE_SOURCE_DIR
include_directories(${CMAKE_SOURCE_DIR}/src/ert)
include_directories(${CMAKE_SOURCE_DIR}/benchmark/kernels)

file(GLOB testsourcefiles ./*.cpp)
file(GLOB exclude_testsourcefiles ./rose_*)
if(exclude_testsourcefiles)
    list(REMOVE_ITEM testsourcefiles ${exclude_testsourcefiles})
endif()
foreach(testsourcefile ${testsourcefiles})
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    add_test("apert_gen_${testname}" ${testname})
    set(testnames ${testnames} ${testname})
endforeach(testsourcefile ${APP_SOURCES})

add_custom_target(apert_gen_all DEPENDS
    ${testnames})