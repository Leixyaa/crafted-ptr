#include "my_pointer_cast.h"
#include "my_make_shared.h"
#include "my_weak_ptr.h"
#include <iostream>
#include <cassert>
#include <map>
#include <set>
#include <algorithm>

// ============================================================================
// 测试用类
// ============================================================================

class Base {
public:
    Base() : value_(0) {
        std::cout << "  [构造] Base()\n";
    }
    
    virtual ~Base() {
        std::cout << "  [析构] ~Base()\n";
    }
    
    virtual void print() const {
        std::cout << "  Base::print() value=" << value_ << "\n";
    }
    
    int value_;
};

class Derived : public Base {
public:
    Derived() : extra_(0) {
        std::cout << "  [构造] Derived()\n";
    }
    
    ~Derived() {
        std::cout << "  [析构] ~Derived()\n";
    }
    
    void print() const override {
        std::cout << "  Derived::print() value=" << value_ << ", extra=" << extra_ << "\n";
    }
    
    int extra_;
};

struct Person {
    std::string name;
    int age;
    
    Person(const std::string& n, int a) : name(n), age(a) {
        std::cout << "  [构造] Person(\"" << name << "\", " << age << ")\n";
    }
    
    ~Person() {
        std::cout << "  [析构] ~Person(\"" << name << "\")\n";
    }
};

// ============================================================================
// 测试函数
// ============================================================================

void test_aliasing_constructor() {
    std::cout << "\n========== 测试 1:别名构造函数 ==========\n";
    
    my::SharedPtr<Person> person = my::make_shared<Person>("Alice", 30);
    std::cout << "person use_count: " << person.use_count() << "\n";
    
    // 使用别名构造访问成员
    my::SharedPtr<std::string> name_ptr(person, &person->name);
    my::SharedPtr<int> age_ptr(person, &person->age);
    
    std::cout << "创建别名指针后:\n";
    std::cout << "  person use_count: " << person.use_count() << "\n";
    std::cout << "  name_ptr use_count: " << name_ptr.use_count() << "\n";
    std::cout << "  age_ptr use_count: " << age_ptr.use_count() << "\n";
    
    assert(person.use_count() == 3);  // person, name_ptr, age_ptr
    assert(name_ptr.use_count() == 3);
    
    // 访问成员
    std::cout << "通过别名指针访问:\n";
    std::cout << "  name: " << *name_ptr << "\n";
    std::cout << "  age: " << *age_ptr << "\n";
    
    // 修改成员
    *age_ptr = 31;
    std::cout << "修改后 person->age: " << person->age << "\n";
    assert(person->age == 31);
    
    // 释放 person,但成员指针仍有效
    person.Reset();
    std::cout << "person.reset() 后:\n";
    std::cout << "  name_ptr use_count: " << name_ptr.use_count() << "\n";
    std::cout << "  name: " << *name_ptr << "\n";
    
    assert(name_ptr.use_count() == 2);
    
    std::cout << " 测试通过:别名构造正确\n";
}

void test_static_pointer_cast() {
    std::cout << "\n========== 测试 2:static_pointer_cast ==========\n";
    
    my::SharedPtr<Derived> derived = my::make_shared<Derived>();
    derived->value_ = 42;
    derived->extra_ = 99;
    
    std::cout << "原始 derived:\n";
    derived->print();
    std::cout << "use_count: " << derived.use_count() << "\n";
    
    // 向上转型:Derived* → Base*
    my::SharedPtr<Base> base = my::static_pointer_cast<Base>(derived);
    
    std::cout << "转型后:\n";
    std::cout << "  base use_count: " << base.use_count() << "\n";
    std::cout << "  derived use_count: " << derived.use_count() << "\n";
    
    assert(base.use_count() == 2);
    assert(derived.use_count() == 2);
    
    // 虚函数调用
    std::cout << "通过 base 调用虚函数:\n";
    base->print();  // 应该调用 Derived::print()
    
    // 向下转型:Base* → Derived*
    my::SharedPtr<Derived> derived2 = my::static_pointer_cast<Derived>(base);
    std::cout << "再次转型后 use_count: " << derived2.use_count() << "\n";
    assert(derived2.use_count() == 3);
    
    std::cout << " 测试通过:static_pointer_cast 正确\n";
}

void test_dynamic_pointer_cast() {
    std::cout << "\n========== 测试 3:dynamic_pointer_cast ==========\n";
    
    // 成功情况:实际是 Derived
    {
        std::cout << "情况 1:转换成功\n";
        my::SharedPtr<Base> base = my::make_shared<Derived>();
        
        my::SharedPtr<Derived> derived = my::dynamic_pointer_cast<Derived>(base);
        
        if (derived) {
            std::cout << "  转换成功!\n";
            std::cout << "  use_count: " << derived.use_count() << "\n";
            derived->print();
            assert(derived.use_count() == 2);
        } else {
            assert(false);  // 不应该失败
        }
    }
    
    // 失败情况:实际只是 Base
    {
        std::cout << "情况 2:转换失败\n";
        my::SharedPtr<Base> base = my::make_shared<Base>();
        
        my::SharedPtr<Derived> derived = my::dynamic_pointer_cast<Derived>(base);
        
        if (derived) {
            assert(false);  // 不应该成功
        } else {
            std::cout << "  转换失败,返回空指针\n";
            std::cout << "  base use_count: " << base.use_count() << "\n";
            assert(base.use_count() == 1);  // base 不受影响
        }
    }
    
    std::cout << " 测试通过:dynamic_pointer_cast 正确\n";
}

class Data {
public:
    Data(int value) : value_(value) {}
    
    int get_value() const { return value_; }
    void set_value(int value) { value_ = value; }
    
private:
    int value_;
};

void test_const_pointer_cast() {
    // std::cout << "\n========== 测试 4:const_pointer_cast ==========\n";
    
    // my::shared_ptr<const int> const_ptr = my::make_shared<int>(42);
    // std::cout << "const_ptr value: " << *const_ptr << "\n";
    
    // // 移除 const
    // my::shared_ptr<int> mutable_ptr = my::const_pointer_cast<int>(const_ptr);
    
    // std::cout << "移除 const 后:\n";
    // std::cout << "  const_ptr use_count: " << const_ptr.use_count() << "\n";
    // std::cout << "  mutable_ptr use_count: " << mutable_ptr.use_count() << "\n";
    
    // assert(const_ptr.use_count() == 2);
    
    // // 修改值
    // *mutable_ptr = 99;
    // std::cout << "修改后:\n";
    // std::cout << "  const_ptr value: " << *const_ptr << "\n";
    // std::cout << "  mutable_ptr value: " << *mutable_ptr << "\n";
    
    // assert(*const_ptr == 99);
    
    // std::cout << " 测试通过:const_pointer_cast 正确\n";

    // 创建一个 const shared_ptr
    my::SharedPtr<const Data> const_ptr = my::make_shared<const Data>(42);
    
    std::cout << "初始值: " << const_ptr->get_value() << "\n";
    
    // const_ptr->set_value(100);  //  编译错误：不能修改 const 对象
    
    // 使用 const_pointer_cast 移除 const
    my::SharedPtr<Data> mutable_ptr = my::const_pointer_cast<Data>(const_ptr);
    
    // 现在可以修改了
    mutable_ptr->set_value(100);
    
    std::cout << "修改后: " << const_ptr->get_value() << "\n";  // 输出: 100
    std::cout << "两个指针指向同一对象: " << (const_ptr.get() == mutable_ptr.get()) << "\n";
}

void test_nullptr_support() {
    std::cout << "\n========== 测试 5:nullptr 支持 ==========\n";
    
    // nullptr 构造
    my::SharedPtr<int> p1 = nullptr;
    assert(!p1);
    assert(p1 == nullptr);
    assert(nullptr == p1);
    
    // nullptr 赋值
    my::SharedPtr<int> p2 = my::make_shared<int>(42);
    assert(p2 != nullptr);
    
    p2 = nullptr;
    assert(p2 == nullptr);
    
    // 比较
    my::SharedPtr<int> p3;
    assert(p3 == nullptr);
    assert(p1 == p3);
    
    std::cout << " 测试通过:nullptr 支持正确\n";
}

void test_comparison_operators() {
    std::cout << "\n========== 测试 6:比较运算符 ==========\n";
    
    my::SharedPtr<int> p1 = my::make_shared<int>(42);
    my::SharedPtr<int> p2 = my::make_shared<int>(42);
    my::SharedPtr<int> p3 = p1;
    
    // 相等比较
    assert(p1 != p2);  // 不同对象
    assert(p1 == p3);  // 同一对象
    
    // 关系比较
    assert((p1 < p2) || (p2 < p1));  // 有序
    assert(!(p1 < p3));  // p1 == p3
    
    std::cout << "比较结果:\n";
    std::cout << "  p1 == p3: " << (p1 == p3) << "\n";
    std::cout << "  p1 != p2: " << (p1 != p2) << "\n";
    std::cout << "  p1 < p2: " << (p1 < p2) << "\n";
    
    std::cout << " 测试通过:比较运算符正确\n";
}

void test_associative_containers() {
    std::cout << "\n========== 测试 7:关联容器 ==========\n";
    
    // std::set
    std::set<my::SharedPtr<int>> set;
    
    auto p1 = my::make_shared<int>(1);
    auto p2 = my::make_shared<int>(2);
    auto p3 = my::make_shared<int>(3);
    
    set.insert(p1);
    set.insert(p2);
    set.insert(p3);
    set.insert(p1);  // 重复插入
    
    std::cout << "set.size(): " << set.size() << "\n";
    assert(set.size() == 3);
    
    // std::map
    std::map<my::SharedPtr<std::string>, int> map;
    
    auto key1 = my::make_shared<std::string>("one");
    auto key2 = my::make_shared<std::string>("two");
    
    map[key1] = 1;
    map[key2] = 2;
    
    std::cout << "map.size(): " << map.size() << "\n";
    assert(map.size() == 2);
    
    std::cout << "map[key1]: " << map[key1] << "\n";
    assert(map[key1] == 1);
    
    std::cout << " 测试通过:关联容器使用正确\n";
}

void test_swap() {
    std::cout << "\n========== 测试 8:swap 函数 ==========\n";
    
    my::SharedPtr<int> p1 = my::make_shared<int>(42);
    my::SharedPtr<int> p2 = my::make_shared<int>(99);
    
    int* addr1 = p1.get();
    int* addr2 = p2.get();
    
    std::cout << "交换前:\n";
    std::cout << "  *p1 = " << *p1 << "\n";
    std::cout << "  *p2 = " << *p2 << "\n";
    
    // 成员 swap
    p1.Swap(p2);
    
    std::cout << "成员 swap 后:\n";
    std::cout << "  *p1 = " << *p1 << "\n";
    std::cout << "  *p2 = " << *p2 << "\n";
    
    assert(*p1 == 99);
    assert(*p2 == 42);
    assert(p1.get() == addr2);
    assert(p2.get() == addr1);
    
    // std::swap
    std::swap(p1, p2);
    
    std::cout << "std::swap 后:\n";
    std::cout << "  *p1 = " << *p1 << "\n";
    std::cout << "  *p2 = " << *p2 << "\n";
    
    assert(*p1 == 42);
    assert(*p2 == 99);
    
    std::cout << " 测试通过:swap 正确\n";
}

void test_complex_scenario() {
    std::cout << "\n========== 测试 9:复杂场景综合 ==========\n";
    
    // 场景:多态 + 类型转换 + 别名构造
    struct Data {
        int value;
        std::string name;
    };
    
    struct Container : public Base {
        Data data;
        
        Container() {
            data.value = 100;
            data.name = "test";
        }
    };
    
    // 创建 Derived 对象
    my::SharedPtr<Container> container = my::make_shared<Container>();
    
    // 向上转型
    my::SharedPtr<Base> base = my::static_pointer_cast<Base>(container);
    
    // 动态向下转型
    my::SharedPtr<Container> container2 = my::dynamic_pointer_cast<Container>(base);
    assert(container2);
    
    // 访问成员
    my::SharedPtr<Data> data_ptr(container2, &container2->data);
    
    std::cout << "引用计数:\n";
    std::cout << "  container: " << container.use_count() << "\n";
    std::cout << "  base: " << base.use_count() << "\n";
    std::cout << "  container2: " << container2.use_count() << "\n";
    std::cout << "  data_ptr: " << data_ptr.use_count() << "\n";
    
    assert(data_ptr.use_count() == 4);
    
    std::cout << "data: value=" << data_ptr->value << ", name=\"" << data_ptr->name << "\"\n";
    
    std::cout << " 测试通过:复杂场景正确\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════╗\n";
    std::cout << "║   Day 7: 完善功能与工程化           ║\n";
    std::cout << "╚══════════════════════════════════════╝\n";
    
    test_aliasing_constructor();
    test_static_pointer_cast();
    test_dynamic_pointer_cast();
    test_const_pointer_cast();
    test_nullptr_support();
    test_comparison_operators();
    test_associative_containers();
    test_swap();
    test_complex_scenario();
    
    return 0;
}