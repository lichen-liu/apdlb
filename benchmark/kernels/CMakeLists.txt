cmake_minimum_required(VERSION 3.7.0)
project(kernels)
include(CTest)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3 -std=c++17)

file(GLOB testsourcefiles ./*.cpp)
foreach(testsourcefile ${testsourcefiles})
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    add_test("kernels_${testname}" ${testname})
    set(testnames ${testnames} ${testname})
endforeach(testsourcefile ${APP_SOURCES})

add_custom_target(kernels_all DEPENDS
    ${testnames})