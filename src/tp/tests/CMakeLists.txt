cmake_minimum_required(VERSION 3.7.0)
project(tp_tests)

include_directories(..)
link_libraries(tp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(wspdr_simple_print_tests wspdr_simple_print_tests.cc)
add_test(tp_wspdr_simple_print_tests wspdr_simple_print_tests)

add_executable(wspdr_collatz_conjecture_tests wspdr_collatz_conjecture_tests.cc)
add_test(tp_wspdr_collatz_conjecture_tests wspdr_collatz_conjecture_tests)

add_executable(suap_simple_print_tests suap_simple_print_tests.cc)
add_test(tp_suap_simple_print_tests suap_simple_print_tests)

add_executable(suap_collatz_conjecture_tests suap_collatz_conjecture_tests.cc)
add_test(tp_suap_collatz_conjecture_tests suap_collatz_conjecture_tests)

add_executable(pool_perf_tests pool_perf_tests.cc)
add_test(tp_pool_perf_tests pool_perf_tests)

# Add test executable here
add_custom_target(tp_tests)
add_dependencies(tp_tests
    wspdr_simple_print_tests
    wspdr_collatz_conjecture_tests
    suap_simple_print_tests
    suap_collatz_conjecture_tests
    pool_perf_tests)