cmake_minimum_required(VERSION 3.10)
project(MySharedPtr_Day6)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 发布版本配置
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译器特定优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -march=native")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
endif()

# 启用线程支持
find_package(Threads REQUIRED)

include_directories(include)

add_executable(test_complete test/test_complete.cc)
target_link_libraries(test_complete Threads::Threads)

add_executable(benchmark test/benchmark.cc)
target_link_libraries(benchmark Threads::Threads)