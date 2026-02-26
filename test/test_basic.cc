#include "../include/my_shared_ptr.h"

#include <cassert>
#include <iostream>

class TestObject {
 public:
  explicit TestObject(int val) : value(val) {
    std::cout << "  [构造] TestObject(" << value << ")\n";
  }

  ~TestObject() { std::cout << "  [析构] ~TestObject(" << value << ")\n"; }

  void Print() const { std::cout << "  TestObject::value = " << value << "\n"; }

  int value;
};

void TestBasicConstruction() {
  std::cout << "\n========== 测试 1: 基本构造 ==========\n";

  {
    my::SharedPtr<TestObject> p1(new TestObject(42));
    std::cout << "p1 引用计数: " << p1.use_count() << "\n";
    assert(p1.use_count() == 1);
    assert(p1.get() != nullptr);
    assert(p1->value == 42);
  }

  std::cout << " 测试通过: 对象正确释放\n";
}

void TestCopyConstruction() {
  std::cout << "\n========== 测试 2: 拷贝构造 ==========\n";

  my::SharedPtr<TestObject> p1(new TestObject(100));
  std::cout << "p1 引用计数: " << p1.use_count() << "\n";

  {
    my::SharedPtr<TestObject> p2(p1);
    std::cout << "p1 引用计数: " << p1.use_count() << "\n";
    std::cout << "p2 引用计数: " << p2.use_count() << "\n";

    assert(p1.use_count() == 2);
    assert(p2.use_count() == 2);
    assert(p1.get() == p2.get());
  }

  std::cout << "p1 引用计数: " << p1.use_count() << "\n";
  assert(p1.use_count() == 1);

  std::cout << " 测试通过: 拷贝构造正确\n";
}

void TestAssignment() {
  std::cout << "\n========== 测试 3: 赋值运算符 ==========\n";

  my::SharedPtr<TestObject> p1(new TestObject(111));
  my::SharedPtr<TestObject> p2(new TestObject(222));

  std::cout << "赋值前:\n";
  std::cout << "  p1 引用计数: " << p1.use_count() << ", 值: " << p1->value << "\n";
  std::cout << "  p2 引用计数: " << p2.use_count() << ", 值: " << p2->value << "\n";

  p2 = p1;

  std::cout << "赋值后:\n";
  std::cout << "  p1 引用计数: " << p1.use_count() << ", 值: " << p1->value << "\n";
  std::cout << "  p2 引用计数: " << p2.use_count() << ", 值: " << p2->value << "\n";

  assert(p1.use_count() == 2);
  assert(p2.use_count() == 2);
  assert(p1.get() == p2.get());

  std::cout << " 测试通过: 赋值运算符正确\n";
}

void TestNullptrHandling() {
  std::cout << "\n========== 测试 4: nullptr 处理 ==========\n";

  my::SharedPtr<TestObject> p1;
  assert(!p1);
  assert(p1.get() == nullptr);
  assert(p1.use_count() == 0);

  my::SharedPtr<TestObject> p2(nullptr);
  assert(!p2);

  my::SharedPtr<TestObject> p3(new TestObject(999));
  p3 = p1;
  assert(!p3);
  assert(p3.use_count() == 0);

  std::cout << " 测试通过: nullptr 处理正确\n";
}

void TestOperators() {
  std::cout << "\n========== 测试 5: 运算符重载 ==========\n";

  my::SharedPtr<TestObject> p(new TestObject(777));

  (*p).Print();
  assert((*p).value == 777);

  p->Print();
  assert(p->value == 777);

  p->value = 888;
  assert(p->value == 888);

  std::cout << " 测试通过: 运算符重载正确\n";
}

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════╗\n";
  std::cout << "║   Day 1: 基础引用计数 SharedPtr      ║\n";
  std::cout << "╚══════════════════════════════════════╝\n";

  TestBasicConstruction();
  TestCopyConstruction();
  TestAssignment();
  TestNullptrHandling();
  TestOperators();

  return 0;
}
