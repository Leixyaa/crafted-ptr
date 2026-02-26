#include "my_shared_ptr.h"

#include <cassert>
#include <cstdio>
#include <iostream>

void CustomDelete(int* p) {
  std::cout << "  [自定义删除器] 释放 int: " << *p << "\n";
  delete p;
}

struct ArrayDeleter {
  void operator()(int* p) const {
    std::cout << "  [ArrayDeleter] 释放数组\n";
    delete[] p;
  }
};

class LoggingDeleter {
 public:
  explicit LoggingDeleter(const char* name) : name_(name) {}

  void operator()(void* p) const {
    std::cout << "  [LoggingDeleter:" << name_ << "] 释放资源\n";
    ::operator delete(p);
  }

 private:
  const char* name_;
};

void TestFunctionPointerDeleter() {
  std::cout << "\n========== 测试 1: 函数指针删除器 ==========\n";

  {
    my::SharedPtr<int> p(new int(42), CustomDelete);
    std::cout << "p 引用计数: " << p.use_count() << "\n";
    std::cout << "p 的值: " << *p << "\n";

    my::SharedPtr<int> p2 = p;
    std::cout << "拷贝后引用计数: " << p.use_count() << "\n";
  }

  std::cout << " 测试通过: 函数指针删除器正确工作\n";
}

void TestFunctorDeleter() {
  std::cout << "\n========== 测试 2: 函数对象删除器 ==========\n";

  {
    int* arr = new int[5]{1, 2, 3, 4, 5};
    my::SharedPtr<int> p(arr, ArrayDeleter());

    std::cout << "数组元素: ";
    for (int i = 0; i < 5; ++i) {
      std::cout << p.get()[i] << " ";
    }
    std::cout << "\n";
  }

  std::cout << " 测试通过: 函数对象删除器正确工作\n";
}

void TestLambdaDeleter() {
  std::cout << "\n========== 测试 3: Lambda 删除器 ==========\n";

  {
    int* arr = new int[10];
    std::cout << "创建大小为 10 的数组\n";

    auto deleter = [](int* p) {
      std::cout << "  [Lambda] 释放数组\n";
      delete[] p;
    };

    my::SharedPtr<int> p(arr, deleter);
    std::cout << "p 引用计数: " << p.use_count() << "\n";
  }

  std::cout << " 测试通过: Lambda 删除器正确工作\n";
}

void TestFileHandle() {
  std::cout << "\n========== 测试 4: 管理文件句柄 ==========\n";

  {
    FILE* f = std::fopen("test_file.txt", "w");
    if (f) {
      std::fprintf(f, "Hello, SharedPtr!");
      std::fclose(f);
      std::cout << "创建测试文件: test_file.txt\n";
    }
  }

  {
    FILE* f = std::fopen("test_file.txt", "r");
    if (!f) {
      std::cout << "无法打开文件\n";
      return;
    }

    my::SharedPtr<FILE> file(f, std::fclose);
    std::cout << "文件已打开, 引用计数: " << file.use_count() << "\n";

    char buffer[100] = {0};
    std::fgets(buffer, sizeof(buffer), file.get());
    std::cout << "文件内容: " << buffer << "\n";

    my::SharedPtr<FILE> file2 = file;
    std::cout << "拷贝后引用计数: " << file.use_count() << "\n";
  }

  std::cout << " 测试通过: 文件句柄正确关闭\n";
}

void TestStatefulDeleter() {
  std::cout << "\n========== 测试 5: 带状态的删除器 ==========\n";

  {
    void* mem1 = ::operator new(100);
    void* mem2 = ::operator new(200);

    my::SharedPtr<void> p1(mem1, LoggingDeleter("Memory-1"));
    my::SharedPtr<void> p2(mem2, LoggingDeleter("Memory-2"));

    std::cout << "p1 引用计数: " << p1.use_count() << "\n";
    std::cout << "p2 引用计数: " << p2.use_count() << "\n";
  }

  std::cout << " 测试通过: 带状态删除器正确工作\n";
}

void TestResetWithDeleter() {
  std::cout << "\n========== 测试 6: Reset() 支持删除器 ==========\n";

  my::SharedPtr<int> p(new int(10), CustomDelete);
  std::cout << "初始值: " << *p << ", 引用计数: " << p.use_count() << "\n";

  auto new_deleter = [](int* ptr) {
    std::cout << "  [新删除器] 释放 int: " << *ptr << "\n";
    delete ptr;
  };

  p.Reset(new int(20), new_deleter);
  std::cout << "Reset 后值: " << *p << ", 引用计数: " << p.use_count() << "\n";

  std::cout << " 测试通过: Reset() 支持删除器\n";
}

void TestVoidPointer() {
  std::cout << "\n========== 测试 7: void* 指针 + 删除器 ==========\n";

  {
    void* mem = new int(999);

    auto deleter = [](void* p) {
      std::cout << "  [void* 删除器] 释放 int: " << *static_cast<int*>(p) << "\n";
      delete static_cast<int*>(p);
    };

    my::SharedPtr<void> p(mem, deleter);
    std::cout << "引用计数: " << p.use_count() << "\n";
  }

  std::cout << " 测试通过: void* 指针正确管理\n";
}

void TestNoDeleteDeleter() {
  std::cout << "\n========== 测试 8: 不释放资源的删除器 ==========\n";

  int stack_value = 123;

  {
    auto no_op_deleter = [](int*) { std::cout << "  [no-op 删除器] 不做任何操作\n"; };

    my::SharedPtr<int> p(&stack_value, no_op_deleter);
    std::cout << "栈变量值: " << *p << "\n";
    std::cout << "引用计数: " << p.use_count() << "\n";
  }

  std::cout << "栈变量仍然有效: " << stack_value << "\n";
  std::cout << " 测试通过: no-op 删除器正确工作\n";
}

class IncompleteType;

IncompleteType* CreateIncomplete();

void TestIncompleteType() {
  std::cout << "测试不完整类型...\n";

  // 这行代码会导致编译错误:
  // my::SharedPtr<IncompleteType> p(CreateIncomplete());

  std::cout << " CheckedDelete 阻止了未定义行为!\n";
}

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════╗\n";
  std::cout << "║   Day 3: 自定义删除器支持            ║\n";
  std::cout << "╚══════════════════════════════════════╝\n";

  TestFunctionPointerDeleter();
  TestFunctorDeleter();
  TestLambdaDeleter();
  TestFileHandle();
  TestStatefulDeleter();
  TestResetWithDeleter();
  TestVoidPointer();
  TestNoDeleteDeleter();

  TestIncompleteType();

  return 0;
}
