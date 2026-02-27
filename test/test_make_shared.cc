#include "my_make_shared.h"
#include "my_weak_ptr.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>


// ============================================================================
// 测试用类
// ============================================================================

class TestObject {
public:
    TestObject(int id, const std::string& name)
        : id_(id), name_(name) {
        std::cout << "  [构造] TestObject(" << id_ << ", \"" << name_ << "\")\n";
        ++object_count;
    }
    
    ~TestObject() {
        std::cout << "  [析构] ~TestObject(" << id_ << ", \"" << name_ << "\")\n";
        --object_count;
    }
    
    int get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    
    int id_;
    std::string name_;
    static int object_count;
};

int TestObject::object_count = 0;

// 用于测试内存延迟释放
class LargeObject {
public:
    LargeObject(int id) : id_(id) {
        std::cout << "  [构造] LargeObject(" << id_ << ") - 分配 1MB\n";
        data_ = new char[1024 * 1024];  // 1MB
        ++object_count;
    }
    
    ~LargeObject() {
        std::cout << "  [析构] ~LargeObject(" << id_ << ") - 释放 1MB\n";
        delete[] data_;
        --object_count;
    }
    
    int id_;
    char* data_;
    static int object_count;
};

int LargeObject::object_count = 0;

// ============================================================================
// 测试函数
// ============================================================================


void test_basic_usage() {
    std::cout << "\n========== 测试 1:基本用法 ==========\n";
    
    my::WeakPtr<TestObject> wp;
    
    {
        my::SharedPtr<TestObject> sp(new TestObject(1, "Alice"));
        std::cout << "shared_ptr use_count: " << sp.use_count() << "\n";
        
        // 从 shared_ptr 创建 weak_ptr
        wp = sp;
        std::cout << "创建 weak_ptr 后:\n";
        std::cout << "  shared_ptr use_count: " << sp.use_count() << "\n";
        std::cout << "  weak_ptr use_count: " << wp.use_count() << "\n";
        std::cout << "  weak_ptr expired: " << wp.expired() << "\n";
        
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
    std::cout << "weak_ptr expired: " << wp.expired() << "\n";
    std::cout << "weak_ptr use_count: " << wp.use_count() << "\n";
    
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

void test_basic_make_shared() {
    std::cout << "\n========== 测试 1:基本用法 ==========\n";
    
    // 使用 make_shared 创建
    auto sp = my::make_shared<TestObject>(1, "Alice");
    
    std::cout << "对象信息:\n";
    std::cout << "  id: " << sp->get_id() << "\n";
    std::cout << "  name: " << sp->get_name() << "\n";
    std::cout << "  use_count: " << sp.use_count() << "\n";
    
    assert(sp->get_id() == 1);
    assert(sp->get_name() == "Alice");
    assert(sp.use_count() == 1);
    
    std::cout << "sp 即将析构...\n";
    sp.Reset();
    
    assert(TestObject::object_count == 0);
    
    std::cout << " 测试通过:基本用法正确\n";
}

void test_make_shared_with_multiple_args() {
    std::cout << "\n========== 测试 2:多参数构造 ==========\n";
    
    struct Point {
        int x, y, z;
        Point(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {
            std::cout << "  [构造] Point(" << x << ", " << y << ", " << z << ")\n";
        }
        ~Point() {
            std::cout << "  [析构] ~Point(" << x << ", " << y << ", " << z << ")\n";
        }
    };
    
    auto sp = my::make_shared<Point>(10, 20, 30);
    
    assert(sp->x == 10);
    assert(sp->y == 20);
    assert(sp->z == 30);
    
    std::cout << " 测试通过:多参数构造正确\n";
}

void test_make_shared_copy_and_move() {
    std::cout << "\n========== 测试 3:拷贝和移动 ==========\n";
    
    auto sp1 = my::make_shared<TestObject>(3, "Bob");
    std::cout << "sp1 use_count: " << sp1.use_count() << "\n";
    
    // 拷贝
    my::SharedPtr<TestObject> sp2 = sp1;
    std::cout << "拷贝后 use_count: " << sp1.use_count() << "\n";
    assert(sp1.use_count() == 2);
    assert(sp2.use_count() == 2);
    
    // 移动
    my::SharedPtr<TestObject> sp3 = std::move(sp1);
    std::cout << "移动后:\n";
    std::cout << "  sp1 use_count: " << (sp1 ? sp1.use_count() : 0) << "\n";
    std::cout << "  sp3 use_count: " << sp3.use_count() << "\n";
    assert(!sp1);
    assert(sp3.use_count() == 2);
    
    std::cout << " 测试通过:拷贝和移动正确\n";
}

void test_make_shared_with_weak_ptr() {
    std::cout << "\n========== 测试 4:与 weak_ptr 配合 ==========\n";
    
    my::WeakPtr<TestObject> wp;
    
    {
        auto sp = my::make_shared<TestObject>(4, "Carol");
        wp = sp;
        
        std::cout << "sp 存在时:\n";
        std::cout << "  sp use_count: " << sp.use_count() << "\n";
        std::cout << "  wp use_count: " << wp.use_count() << "\n";
        std::cout << "  wp expired: " << wp.expired() << "\n";
        
        assert(sp.use_count() == 1);
        assert(!wp.expired());
        
        std::cout << "sp 即将析构...\n";
    }
    
    std::cout << "sp 析构后:\n";
    std::cout << "  wp expired: " << wp.expired() << "\n";
    std::cout << "  wp use_count: " << wp.use_count() << "\n";
    
    assert(wp.expired());
    assert(wp.use_count() == 0);
    
    // 尝试 lock
    auto locked = wp.lock();
    assert(!locked);
    
    std::cout << " 测试通过:与 weak_ptr 配合正确\n";
}

void test_make_shared_memory_layout() {
    std::cout << "\n========== 测试 5:内存布局验证 ==========\n";
    
    auto sp1 = my::make_shared<TestObject>(5, "Dave");
    auto sp2 = my::make_shared<TestObject>(6, "Eve");
    
    // 验证对象地址不同
    TestObject* p1 = sp1.get();
    TestObject* p2 = sp2.get();
    
    std::cout << "对象地址:\n";
    std::cout << "  sp1: " << static_cast<void*>(p1) << "\n";
    std::cout << "  sp2: " << static_cast<void*>(p2) << "\n";
    
    assert(p1 != p2);
    
    std::cout << " 测试通过:内存布局正确\n";
}

void test_make_shared_with_vector() {
    std::cout << "\n========== 测试 6:在容器中使用 ==========\n";
    
    std::vector<my::SharedPtr<TestObject>> vec;
    
    for (int i = 0; i < 5; ++i) {
        vec.push_back(my::make_shared<TestObject>(i, "Item" + std::to_string(i)));
    }
    
    std::cout << "容器中有 " << vec.size() << " 个对象\n";
    std::cout << "活跃对象数: " << TestObject::object_count << "\n";
    
    assert(TestObject::object_count == 5);
    
    vec.clear();
    
    std::cout << "清空容器后,活跃对象数: " << TestObject::object_count << "\n";
    assert(TestObject::object_count == 0);
    
    std::cout << " 测试通过:在容器中使用正确\n";
}

void test_make_shared_exception_safety() {
    std::cout << "\n========== 测试 7:异常安全 ==========\n";
    
    class MayThrow {
    public:
        MayThrow(bool should_throw) {
            std::cout << "  [构造] MayThrow\n";
            if (should_throw) {
                throw std::runtime_error("构造失败!");
            }
        }
        ~MayThrow() {
            std::cout << "  [析构] ~MayThrow\n";
        }
    };
    
    // 正常构造
    try {
        auto sp = my::make_shared<MayThrow>(false);
        std::cout << "正常构造成功\n";
    } catch (...) {
        assert(false);  // 不应该抛异常
    }
    
    // 构造失败
    try {
        auto sp = my::make_shared<MayThrow>(true);
        assert(false);  // 不应该到这里
    } catch (const std::runtime_error& e) {
        std::cout << "捕获异常: " << e.what() << "\n";
    }
    
    std::cout << " 测试通过:异常安全\n";
}

void demonstrate_delayed_memory_release() {
    std::cout << "\n========== 演示:内存延迟释放(make_shared 的缺点) ==========\n";
    
    my::WeakPtr<LargeObject> wp;
    
    {
        std::cout << "创建 make_shared<LargeObject>...\n";
        auto sp = my::make_shared<LargeObject>(1);
        wp = sp;
        
        std::cout << "释放 shared_ptr...\n";
        sp.Reset();
        
        std::cout << "对象已析构,但内存未释放!\n";
        std::cout << "weak_ptr 仍然存活,活跃对象数: " << LargeObject::object_count << "\n";
        
        // 此时:
        // - LargeObject 已析构(dispose() 调用)
        // - 但 1MB 内存还未释放!(因为 weak_ptr 还在)
        
        std::cout << "等待 3 秒...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cout << "weak_ptr 即将析构...\n";
    }
    
    std::cout << "内存现在才真正释放!\n";
    
    std::cout << " 教训:如果对象很大且有 weak_ptr,make_shared 会延迟释放内存\n";
}

// ============================================================================
// 主函数
// ============================================================================

void test_void_pointer() {
    std::cout << "\n========== 测试 7:void* 指针 + 删除器 ==========\n";
    
    {
        // 分配 int,但用 void* 保存
        void* mem = new int(999);
        
        // 删除器知道真实类型
        auto deleter = [](void* p) {
            std::cout << "  [void* 删除器] 释放 int: " 
                      << *static_cast<int*>(p) << "\n";
            delete static_cast<int*>(p);
        };
        
        my::SharedPtr<void> p(mem, deleter);
        std::cout << "引用计数: " << p.use_count() << "\n";
    }
    
    std::cout << " 测试通过:void* 指针正确管理\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   Day 6: make_shared 性能优化       ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";

    test_void_pointer();
    test_basic_usage();

    test_basic_make_shared();
    test_make_shared_with_multiple_args();
    test_make_shared_copy_and_move();
    test_make_shared_with_weak_ptr();
    test_make_shared_memory_layout();
    test_make_shared_with_vector();
    test_make_shared_exception_safety();
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    demonstrate_delayed_memory_release();
    std::cout << std::string(50, '=') << "\n";
    
    return 0;
}