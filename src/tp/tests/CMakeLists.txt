cmake_minimum_required(VERSION 3.7.0)
project(tp_tests)

add_compile_options(-Werror -Wall -Wno-missing-braces -O3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(..)

add_executable(wspdr_pool_tests wspdr_pool_tests.cc)
add_test(tp_wspdr_pool_tests wspdr_pool_tests)

add_executable(suap_pool_tests suap_pool_tests.cc)
add_test(tp_suap_pool_tests suap_pool_tests)

add_executable(pool_perf_tests pool_perf_tests.cc)
add_test(tp_pool_perf_tests pool_perf_tests)

# Add test executable here
add_custom_target(tp_tests DEPENDS
    wspdr_pool_tests
    suap_pool_tests
    pool_perf_tests)