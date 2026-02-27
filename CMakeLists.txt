cmake_minimum_required(VERSION 3.10)
project(MySharedPtr_Day6)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优化选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# 启用线程支持
find_package(Threads REQUIRED)

include_directories(include)

# make_shared 功能测试
add_executable(test_make_shared test/test_make_shared.cc)
target_link_libraries(test_make_shared Threads::Threads)

# 性能对比测试
# add_executable(benchmark test/benchmark.cc)
# target_link_libraries(benchmark Threads::Threads)
