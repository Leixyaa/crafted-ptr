cmake_minimum_required(VERSION 3.10)
project(MySharedPtr_Day5)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用线程支持
find_package(Threads REQUIRED)

include_directories(include)

# weak_ptr 功能测试
add_executable(test_weak_ptr test/test_weak_ptr.cc)
target_link_libraries(test_weak_ptr Threads::Threads)

# 循环引用案例测试
add_executable(test_cycle test/test_cycle.cc)
target_link_libraries(test_cycle Threads::Threads)