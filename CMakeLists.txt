cmake_minimum_required (VERSION 3.12)

project (spinlock-benchmark)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
add_subdirectory(benchmark)

add_executable(spinlock-benchmark main.cpp)
target_compile_features(spinlock-benchmark PRIVATE cxx_std_20)
target_link_libraries(spinlock-benchmark PRIVATE benchmark::benchmark)