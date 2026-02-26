#include "my_shared_ptr.h"

#include <cassert>
#include <iostream>

class Animal {
 public:
  explicit Animal(const char* name) : name_(name) {
    std::cout << "  [构造] Animal(" << name_ << ")\n";
  }

  virtual ~Animal() { std::cout << "  [析构] ~Animal(" << name_ << ")\n"; }

  virtual void Speak() const { std::cout << "  " << name_ << " makes a sound\n"; }

 protected:
  const char* name_;
};

class Dog : public Animal {
 public:
  explicit Dog(const char* name) : Animal(name) {
    std::cout << "  [构造] Dog(" << name_ << ")\n";
  }

  ~Dog() override { std::cout << "  [析构] ~Dog(" << name_ << ")\n"; }

  void Speak() const override { std::cout << "  " << name_ << " barks: Woof!\n"; }
};

class Cat : public Animal {
 public:
  explicit Cat(const char* name) : Animal(name) {
    std::cout << "  [构造] Cat(" << name_ << ")\n";
  }

  ~Cat() override { std::cout << "  [析构] ~Cat(" << name_ << ")\n"; }

  void Speak() const override { std::cout << "  " << name_ << " meows: Meow!\n"; }
};

void TestBasicControlBlock() {
  std::cout << "\n========== 测试 1: 基本控制块功能 ==========\n";

  {
    my::SharedPtr<Dog> dog1(new Dog("Buddy"));
    std::cout << "dog1 引用计数: " << dog1.use_count() << "\n";
    assert(dog1.use_count() == 1);

    {
      my::SharedPtr<Dog> dog2(dog1);
      std::cout << "dog1 引用计数: " << dog1.use_count() << "\n";
      std::cout << "dog2 引用计数: " << dog2.use_count() << "\n";
      assert(dog1.use_count() == 2);
      assert(dog2.use_count() == 2);

      dog1->Speak();
      dog2->Speak();
    }

    std::cout << "dog1 引用计数: " << dog1.use_count() << "\n";
    assert(dog1.use_count() == 1);
  }

  std::cout << " 测试通过: 控制块正确管理生命周期\n";
}

void TestTypeErasure() {
  std::cout << "\n========== 测试 2: 类型擦除 ==========\n";

  std::cout << "--- Dog → Animal ---\n";
  {
    my::SharedPtr<Animal> animal(new Dog("Max"));
    animal->Speak();

    std::cout << "引用计数: " << animal.use_count() << "\n";
  }

  std::cout << "\n--- Cat → Animal ---\n";
  {
    my::SharedPtr<Animal> animal(new Cat("Whiskers"));
    animal->Speak();
  }

  std::cout << " 测试通过: 类型擦除正确工作\n";
}

void TestPolymorphicContainer() {
  std::cout << "\n========== 测试 3: 多态容器 ==========\n";

  {
    my::SharedPtr<Animal> pets[3] = {
        my::SharedPtr<Animal>(new Dog("Buddy")),
        my::SharedPtr<Animal>(new Cat("Whiskers")),
        my::SharedPtr<Animal>(new Dog("Charlie")),
    };

    std::cout << "\n所有宠物说话:\n";
    for (int i = 0; i < 3; ++i) {
      pets[i]->Speak();
    }

    std::cout << "\n引用计数:\n";
    for (int i = 0; i < 3; ++i) {
      std::cout << "  pet[" << i << "]: " << pets[i].use_count() << "\n";
    }
  }

  std::cout << " 测试通过: 多态容器正确释放\n";
}

void TestConversion() {
  std::cout << "\n========== 测试 4: 隐式类型转换 ==========\n";

  my::SharedPtr<Dog> dog(new Dog("Rocky"));
  std::cout << "dog 引用计数: " << dog.use_count() << "\n";

  my::SharedPtr<Animal> animal = dog;
  std::cout << "转换后:\n";
  std::cout << "  dog 引用计数: " << dog.use_count() << "\n";
  std::cout << "  animal 引用计数: " << animal.use_count() << "\n";

  assert(dog.use_count() == 2);
  assert(animal.use_count() == 2);
  assert(dog.get() == animal.get());

  animal->Speak();
  dog->Speak();

  std::cout << " 测试通过: 类型转换正确\n";
}

void TestReset() {
  std::cout << "\n========== 测试 5: Reset() 功能 ==========\n";

  my::SharedPtr<Dog> p1(new Dog("Toby"));
  std::cout << "p1 引用计数: " << p1.use_count() << "\n";

  my::SharedPtr<Dog> p2 = p1;
  std::cout << "拷贝后 p1 引用计数: " << p1.use_count() << "\n";

  p1.Reset();
  std::cout << "p1.Reset() 后:\n";
  std::cout << "  p1 引用计数: " << p1.use_count() << "\n";
  std::cout << "  p2 引用计数: " << p2.use_count() << "\n";
  assert(!p1);
  assert(p2.use_count() == 1);

  p2.Reset(new Dog("Bella"));
  std::cout << "p2.Reset(new Dog) 后:\n";
  std::cout << "  p2 引用计数: " << p2.use_count() << "\n";

  std::cout << " 测试通过: Reset() 正确\n";
}

int main() {
  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════╗\n";
  std::cout << "║   Day 2: 控制块分离架构              ║\n";
  std::cout << "╚══════════════════════════════════════╝\n";

  TestBasicControlBlock();
  TestTypeErasure();
  TestPolymorphicContainer();
  TestConversion();
  TestReset();

  return 0;
}
