#include "my_shared_ptr.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <cassert>
#include <chrono>

// ============================================================================
// 测试用类
// ============================================================================

class TestObject {
public:
    TestObject(int id) : id_(id) {
        std::cout << "  [构造] TestObject #" << id_ << "\n";
        ++object_count;
    }
    
    ~TestObject() {
        std::cout << "  [析构] ~TestObject #" << id_ << "\n";
        --object_count;
    }
    
    void do_work() const {
        // 模拟工作
        volatile int dummy = 0;
        for (int i = 0; i < 1000; ++i) {
            dummy += i;
        }
    }
    
    int id_;
    static std::atomic<int> object_count;
};

std::atomic<int> TestObject::object_count{0};

// ============================================================================
// 测试函数
// ============================================================================

void test_concurrent_copy() {
    std::cout << "\n========== 测试 1:并发拷贝 shared_ptr ==========\n";
    
    constexpr int NUM_THREADS = 10;
    constexpr int COPIES_PER_THREAD = 1000;
    
    my::SharedPtr<TestObject> source(new TestObject(1));
    std::cout << "初始引用计数: " << source.use_count() << "\n";
    
    // 多个线程并发拷贝同一个 shared_ptr
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&source]() {
            std::vector<my::SharedPtr<TestObject>> local_copies;
            
            for (int j = 0; j < COPIES_PER_THREAD; ++j) {
                // 每个线程创建自己的拷贝
                local_copies.push_back(source);
            }
            
            // local_copies 析构,释放所有拷贝
        });
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "最终引用计数: " << source.use_count() << "\n";
    assert(source.use_count() == 1);  //  应该回到 1
    
    std::cout << "测试通过:并发拷贝正确\n";
}

void test_concurrent_destruction() {
    std::cout << "\n========== 测试 2:并发析构 ==========\n";
    
    constexpr int NUM_THREADS = 20;
    
    std::vector<std::thread> threads;
    my::SharedPtr<TestObject> source(new TestObject(2));
    
    std::cout << "初始引用计数: " << source.use_count() << "\n";
    
    // 创建多个拷贝
    std::vector<my::SharedPtr<TestObject>> copies;
    for (int i = 0; i < NUM_THREADS; ++i) {
        copies.push_back(source);
    }
    
    std::cout << "拷贝后引用计数: " << source.use_count() << "\n";
    
    // 每个线程负责析构一个拷贝
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([copy = std::move(copies[i])]() mutable {
            // copy 在这里析构
            copy.Reset();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "最终引用计数: " << source.use_count() << "\n";
    assert(source.use_count() == 1);
    
    std::cout << " 测试通过:并发析构正确\n";
}

void test_concurrent_access() {
    std::cout << "\n========== 测试 3:并发访问对象 ==========\n";
    
    constexpr int NUM_THREADS = 8;
    constexpr int ACCESSES = 10000;
    
    my::SharedPtr<TestObject> ptr(new TestObject(3));
    std::vector<std::thread> threads;
    
    // 多个线程并发访问对象(不修改 shared_ptr 本身)
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([copy = ptr]() {
            for (int j = 0; j < ACCESSES; ++j) {
                copy->do_work();  // 访问对象
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "引用计数: " << ptr.use_count() << "\n";
    assert(ptr.use_count() == 1);
    
    std::cout << " 测试通过:并发访问正确\n";
}

void test_passing_between_threads() {
    std::cout << "\n========== 测试 4:在线程间传递 ==========\n";
    
    constexpr int NUM_ITERATIONS = 1000;
    
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        my::SharedPtr<TestObject> ptr(new TestObject(1000 + iter));
        
        // 线程 A 创建后传给线程 B
        std::thread thread_b;
        
        std::thread thread_a([&ptr, &thread_b]() {
            // 拷贝 ptr 并传给线程 B
            thread_b = std::thread([copy = ptr]() {
                copy->do_work();
                // copy 在这里析构
            });
        });
        
        thread_a.join();
        thread_b.join();
        
        // ptr 在这里析构(可能最后一个引用)
    }
    
    std::cout << "活跃对象数: " << TestObject::object_count.load() << "\n";
    assert(TestObject::object_count == 0);  //  所有对象都被释放
    
    std::cout << " 测试通过:线程间传递正确\n";
}

void test_stress_refcount() {
    std::cout << "\n========== 测试 5:引用计数压力测试 ==========\n";
    
    constexpr int NUM_THREADS = 50;
    constexpr int OPERATIONS = 10000;
    
    my::SharedPtr<TestObject> source(new TestObject(5));
    std::atomic<int> max_observed_count{0};
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&source, &max_observed_count]() {
            for (int j = 0; j < OPERATIONS; ++j) {
                // 创建临时拷贝
                my::SharedPtr<TestObject> temp = source;
                
                // 记录观察到的最大引用计数
                int current = temp.use_count();
                int prev_max = max_observed_count.load();
                while (current > prev_max && 
                       !max_observed_count.compare_exchange_weak(prev_max, current)) {
                    // 重试
                }
                
                // temp 析构
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "最大观察到的引用计数: " << max_observed_count.load() << "\n";
    std::cout << "最终引用计数: " << source.use_count() << "\n";
    assert(source.use_count() == 1);
    
    std::cout << " 测试通过:引用计数始终正确\n";
}

void test_no_memory_leak() {
    std::cout << "\n========== 测试 6:无内存泄漏 ==========\n";
    
    constexpr int NUM_ROUNDS = 100;
    constexpr int NUM_THREADS = 10;
    
    std::cout << "初始对象数: " << TestObject::object_count.load() << "\n";
    
    for (int round = 0; round < NUM_ROUNDS; ++round) {
        my::SharedPtr<TestObject> ptr(new TestObject(6000 + round));
        
        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([copy = ptr]() {
                copy->do_work();
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        // ptr 和所有 copy 都析构了
    }
    
    std::cout << "最终对象数: " << TestObject::object_count.load() << "\n";
    assert(TestObject::object_count == 0);
    
    std::cout << " 测试通过:无内存泄漏\n";
}

void test_performance_comparison() {
    std::cout << "\n========== 测试 7:性能对比 ==========\n";
    
    constexpr int NUM_OPERATIONS = 1000000;
    
    // 单线程性能
    {
        my::SharedPtr<TestObject> ptr(new TestObject(7));
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            my::SharedPtr<TestObject> temp = ptr;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "单线程 " << NUM_OPERATIONS << " 次拷贝: " 
                  << duration.count() << " ms\n";
    }
    
    // 多线程性能
    {
        my::SharedPtr<TestObject> ptr(new TestObject(8));
        constexpr int NUM_THREADS = 4;
        constexpr int OPS_PER_THREAD = NUM_OPERATIONS / NUM_THREADS;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([copy = ptr]() {
                for (int i = 0; i < OPS_PER_THREAD; ++i) {
                    my::SharedPtr<TestObject> temp = copy;
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << NUM_THREADS << "线程 " << NUM_OPERATIONS << " 次拷贝: " 
                  << duration.count() << " ms\n";
    }
    
    std::cout << " 性能测试完成\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   Day 4: 线程安全的引用计数          ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    
    test_concurrent_copy();
    test_concurrent_destruction();
    test_concurrent_access();
    test_passing_between_threads();
    test_stress_refcount();
    test_no_memory_leak();
    test_performance_comparison();
    
    return 0;
}