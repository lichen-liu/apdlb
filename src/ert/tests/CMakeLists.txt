cmake_minimum_required(VERSION 3.7.0)
project(ert_tests)
include(CTest)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(..)

file(GLOB testsourcefiles ./*.cc)
foreach(testsourcefile ${testsourcefiles})
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    add_test("ert_${testname}" ${testname})
    set(testnames ${testnames} ${testname})
endforeach(testsourcefile ${APP_SOURCES})

add_custom_target(ert_tests DEPENDS
    ${testnames})