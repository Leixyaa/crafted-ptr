#include "my_shared_ptr.h"
#include "my_weak_ptr.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

// ============================================================================
// 测试用类
// ============================================================================

class TestObject {
public:
    TestObject(int id) : id_(id), data_(0) {
        std::cout << "  [构造] TestObject #" << id_ << "\n";
        ++object_count;
    }
    
    ~TestObject() {
        std::cout << "  [析构] ~TestObject #" << id_ << "\n";
        --object_count;
    }
    
    void set_data(int value) { data_ = value; }
    int get_data() const { return data_; }
    
    int id_;
    int data_;
    static int object_count;
};

int TestObject::object_count = 0;

// ============================================================================
// 测试函数
// ============================================================================

void test_basic_usage() {
    std::cout << "\n========== 测试 1:基本用法 ==========\n";
    
    my::WeakPtr<TestObject> wp;
    
    {
        my::SharedPtr<TestObject> sp(new TestObject(1));
        std::cout << "SharedPtr use_count: " << sp.use_count() << "\n";
        
        // 从 SharedPtr 创建 WeakPtr
        wp = sp;
        std::cout << "创建 WeakPtr 后:\n";
        std::cout << "  SharedPtr use_count: " << sp.use_count() << "\n";
        std::cout << "  WeakPtr use_count: " << wp.use_count() << "\n";
        std::cout << "  WeakPtr expired: " << wp.expired() << "\n";
        
        assert(sp.use_count() == 1);  //  弱引用不增加强引用计数
        assert(wp.use_count() == 1);
        assert(!wp.expired());
        
        // 使用 lock() 访问对象
        if (my::SharedPtr<TestObject> locked = wp.lock()) {
            std::cout << "lock() 成功,访问对象: id=" << locked->id_ << "\n";
            std::cout << "  当前 use_count: " << locked.use_count() << "\n";
            assert(locked.use_count() == 2);  // sp + locked
        }
        
        std::cout << "sp 即将析构...\n";
    }
    
    std::cout << "sp 已析构\n";
    std::cout << "WeakPtr expired: " << wp.expired() << "\n";
    std::cout << "WeakPtr use_count: " << wp.use_count() << "\n";
    
    assert(wp.expired());
    assert(wp.use_count() == 0);
    
    // 尝试 lock()
    my::SharedPtr<TestObject> locked = wp.lock();
    std::cout << "lock() 后: " << (locked ? "成功" : "失败(返回空指针)") << "\n";
    assert(!locked);
    
    std::cout << "活跃对象数: " << TestObject::object_count << "\n";
    assert(TestObject::object_count == 0);
    
    std::cout << " 测试通过:基本用法正确\n";
}

void test_WeakPtr_lifetime() {
    std::cout << "\n========== 测试 2:WeakPtr 生命周期 ==========\n";
    
    my::WeakPtr<TestObject> wp1, wp2;
    
    {
        my::SharedPtr<TestObject> sp(new TestObject(2));
        wp1 = sp;
        wp2 = wp1;  // 拷贝 WeakPtr
        
        std::cout << "创建 2 个 WeakPtr:\n";
        std::cout << "  SharedPtr use_count: " << sp.use_count() << "\n";
        std::cout << "  wp1 use_count: " << wp1.use_count() << "\n";
        std::cout << "  wp2 use_count: " << wp2.use_count() << "\n";
        
        assert(sp.use_count() == 1);
        assert(!wp1.expired());
        assert(!wp2.expired());
    }
    
    std::cout << "SharedPtr 析构后:\n";
    std::cout << "  wp1 expired: " << wp1.expired() << "\n";
    std::cout << "  wp2 expired: " << wp2.expired() << "\n";
    
    assert(wp1.expired());
    assert(wp2.expired());
    
    std::cout << " 测试通过:WeakPtr 生命周期正确\n";
}

void test_lock_race_condition() {
    std::cout << "\n========== 测试 3:多线程 lock() 竞争 ==========\n";
    
    constexpr int NUM_THREADS = 10;
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};
    
    my::SharedPtr<TestObject> sp(new TestObject(3));
    my::WeakPtr<TestObject> wp = sp;
    
    std::vector<std::thread> threads;
    
    // 启动多个线程尝试 lock()
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&wp, &success_count, &fail_count]() {
            for (int j = 0; j < 100000; ++j) {
                my::SharedPtr<TestObject> locked = wp.lock();
                if (locked) {
                    success_count++;
                    locked->set_data(locked->get_data() + 1);
                } else {
                    fail_count++;
                }
                
                // 模拟工作
               // std::this_thread::yield();
            }
        });
    }
    
    // 让线程运行一会儿
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 释放 SharedPtr
    std::cout << "释放 SharedPtr...\n";
    sp.Reset();
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "成功 lock(): " << success_count.load() << "\n";
    std::cout << "失败 lock(): " << fail_count.load() << "\n";
    std::cout << "总计: " << (success_count + fail_count) << "\n";
    
    assert(success_count > 0);
    assert(fail_count > 0);  // sp.Reset() 后应该有失败
    
    std::cout << " 测试通过:多线程 lock() 安全\n";
}

void test_multiple_weak_from_same_shared() {
    std::cout << "\n========== 测试 4:多个 WeakPtr 共享 SharedPtr ==========\n";
    
    my::SharedPtr<TestObject> sp(new TestObject(4));
    std::vector<my::WeakPtr<TestObject>> WeakPtrs;
    
    // 创建 100 个 WeakPtr
    for (int i = 0; i < 100; ++i) {
        WeakPtrs.push_back(sp);
    }
    
    std::cout << "创建 100 个 WeakPtr:\n";
    std::cout << "  SharedPtr use_count: " << sp.use_count() << "\n";
    assert(sp.use_count() == 1);  //  弱引用不影响强引用计数
    
    // 所有 WeakPtr 都应该有效
    for (const auto& wp : WeakPtrs) {
        assert(!wp.expired());
        assert(wp.use_count() == 1);
    }
    
    // 释放 SharedPtr
    sp.Reset();
    
    std::cout << "SharedPtr 释放后:\n";
    for (const auto& wp : WeakPtrs) {
        assert(wp.expired());
        assert(wp.use_count() == 0);
    }
    
    std::cout << " 测试通过:多个 WeakPtr 正确\n";
}

void test_reset_and_swap() {
    std::cout << "\n========== 测试 5:Reset() 和 swap() ==========\n";
    
    my::SharedPtr<TestObject> sp1(new TestObject(5));
    my::SharedPtr<TestObject> sp2(new TestObject(6));
    
    my::WeakPtr<TestObject> wp1 = sp1;
    my::WeakPtr<TestObject> wp2 = sp2;
    
    std::cout << "交换前:\n";
    std::cout << "  wp1 指向对象 #" << wp1.lock()->id_ << "\n";
    std::cout << "  wp2 指向对象 #" << wp2.lock()->id_ << "\n";
    
    wp1.swap(wp2);
    
    std::cout << "交换后:\n";
    std::cout << "  wp1 指向对象 #" << wp1.lock()->id_ << "\n";
    std::cout << "  wp2 指向对象 #" << wp2.lock()->id_ << "\n";
    
    assert(wp1.lock()->id_ == 6);
    assert(wp2.lock()->id_ == 5);
    
    // 测试 Reset
    wp1.Reset();
    std::cout << "wp1.Reset() 后:\n";
    std::cout << "  wp1 expired: " << wp1.expired() << "\n";
    assert(wp1.expired());
    
    std::cout << " 测试通过:Reset() 和 swap() 正确\n";
}

void test_WeakPtr_copy_and_move() {
    std::cout << "\n========== 测试 6:WeakPtr 拷贝和移动 ==========\n";
    
    my::SharedPtr<TestObject> sp(new TestObject(7));
    my::WeakPtr<TestObject> wp1 = sp;
    
    // 拷贝构造
    my::WeakPtr<TestObject> wp2(wp1);
    assert(wp2.use_count() == 1);
    assert(!wp2.expired());
    
    // 拷贝赋值
    my::WeakPtr<TestObject> wp3;
    wp3 = wp2;
    assert(wp3.use_count() == 1);
    
    // 移动构造
    my::WeakPtr<TestObject> wp4(std::move(wp2));
    assert(wp4.use_count() == 1);
    assert(wp2.expired());  // wp2 被移走
    
    // 移动赋值
    my::WeakPtr<TestObject> wp5;
    wp5 = std::move(wp3);
    assert(wp5.use_count() == 1);
    assert(wp3.expired());
    
    std::cout << " 测试通过:拷贝和移动语义正确\n";
}

void test_lock_with_expired() {
    std::cout << "\n========== 测试 7:expired() 优化 ==========\n";
    
    my::WeakPtr<TestObject> wp;
    
    {
        my::SharedPtr<TestObject> sp(new TestObject(8));
        wp = sp;
    }
    
    // 推荐用法
    if (!wp.expired()) {
        my::SharedPtr<TestObject> sp = wp.lock();
        if (sp) {
            std::cout << "访问对象\n";
        }
    } else {
        std::cout << "对象已过期,跳过 lock()\n";
    }
    
    assert(wp.expired());
    
    std::cout << " 测试通过:expired() 正确\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   Day 5: WeakPtr 与循环引用         ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    
    test_basic_usage();
    test_WeakPtr_lifetime();
    test_lock_race_condition();
    test_multiple_weak_from_same_shared();
    test_reset_and_swap();
    test_WeakPtr_copy_and_move();
    test_lock_with_expired();
    
    return 0;
}