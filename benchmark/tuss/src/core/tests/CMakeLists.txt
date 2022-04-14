cmake_minimum_required(VERSION 3.7.0)
project(core_tests)

include_directories(..)
link_libraries(core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB testsourcefiles ./*.cc)
foreach(testsourcefile ${testsourcefiles})
    get_filename_component(testname ${testsourcefile} NAME_WE)
    add_executable(${testname} ${testsourcefile})
    add_test("core_tests_${testname}" ${testname})
    set(testnames ${testnames} ${testname})
endforeach(testsourcefile ${APP_SOURCES})

add_custom_target(core_tests DEPENDS
    ${testnames})