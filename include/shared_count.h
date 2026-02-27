#ifndef MY_SHARED_COUNT_HPP_
#define MY_SHARED_COUNT_HPP_

#include <cstddef>
#include <stdint.h>
#include <utility>

#include "sp_counted_base.h"
#include "sp_counted_impl.h"

namespace my {

template <typename T>
class SharedPtr;
template <typename T>
class WeakPtr;

namespace detail {

//  标签类型:表示 inplace 构造
template<typename T>
struct sp_inplace_tag {};

// 用于区分构造函数类型的内部标签
struct sp_deleter_tag {};


class SharedCount;

// ============================================================================
// WeakCount: 弱引用计数辅助类(参考 Boost)
// ============================================================================

class WeakCount {
 public:
  ////////// 构造函数 //////////

  // 默认构造（空WeakCount）
  WeakCount() noexcept : control_block_(nullptr) {}

  // 从 shared_count 构造(weak_ptr 从 shared_ptr 创建时用)
  WeakCount(const SharedCount& other) noexcept;  // 延迟定义，因为需要SharedCount

  // 拷贝构造
  WeakCount(const WeakCount& other) noexcept : control_block_(other.control_block_) {
    if (control_block_) {
      control_block_->WeakAddRef();  // 增加弱引用计数
    }
  }

  // 移动构造
  WeakCount(WeakCount&& other) noexcept : control_block_(other.control_block_) {
    other.control_block_ = nullptr;
  }

  ////////// 析构函数 //////////
  ~WeakCount() noexcept {
    if (control_block_) {
      control_block_->WeakRelease();
    }
  }

  ////////// 赋值运算 //////////
  WeakCount& operator=(const SharedCount& other) noexcept;

  WeakCount& operator=(const WeakCount& other) noexcept {
    if (control_block_ != other.control_block_) {
      SpCountedBase* new_control_block = other.control_block_;
      if (new_control_block) {
        new_control_block->WeakAddRef();
      }
      if (control_block_) {
        control_block_->WeakRelease();
      }
      control_block_ = new_control_block;
    }
    return *this;
  }

  WeakCount& operator=(WeakCount&& other) noexcept {
    WeakCount(std::move(other)).swap(*this);
    return *this;
  }

  /////////// 其他操作 //////////

  void Swap(WeakCount& other) noexcept {
    SpCountedBase* tmp = control_block_;
    control_block_ = other.control_block_;
    other.control_block_ = tmp;
  }

  void swap(WeakCount& other) noexcept { Swap(other); }

  int64_t use_count() const noexcept {
    return control_block_ ? control_block_->use_count() : 0;
  }

  bool empty() const noexcept { return control_block_ == nullptr; }

  bool OwnerBefore(const WeakCount& other) const noexcept {
    return control_block_ < other.control_block_;
  }

  //  友元声明
  friend class SharedCount;
  template <typename T>
  friend class my::SharedPtr;
  template <typename T>
  friend class my::WeakPtr;

 private:
  SpCountedBase* control_block_;
};

// ============================================================================
// SharedCount: 引用计数管理器 (拥有控制块)
// ============================================================================

class SharedCount {
 public:
  SharedCount() noexcept : control_block_(nullptr) {}

  template <typename T>
  explicit SharedCount(T* ptr) : control_block_(nullptr) {
    if (ptr) {
      control_block_ = new SpCountedImplPointer<T>(ptr);
    }
  }

  template <typename P, typename D>
  explicit SharedCount(sp_deleter_tag, P ptr, D deleter) : control_block_(nullptr) {
    if (ptr) {
      control_block_ = new SpCountedImplPointerDeleter<P, D>(ptr, deleter);
    }
  }

  //  Inplace 构造(用于 make_shared)
  // 使用 SFINAE 确保只匹配 sp_inplace_tag
  template <typename T, typename... Args> 
  explicit SharedCount(sp_inplace_tag<T> tag, Args&&... args) 
    : control_block_(nullptr) {
      (void) tag; // 避免未使用警告
      typedef SpCountedImplPdi<T> ImplType;
      control_block_ = new ImplType(std::forward<Args>(args)...);
    }


  SharedCount(const SharedCount& other) noexcept
      : control_block_(other.control_block_) {
    if (control_block_) {
      control_block_->AddRefCopy();
    }
  }

  // 从 weak_count 构造(用于 weak_ptr::lock())
  explicit SharedCount(const WeakCount& other) : control_block_(other.control_block_) {
    if (control_block_) {
      if (!control_block_->AddRefLock()) {
        // 对象已销毁(use_count_ == 0)
        control_block_ = nullptr;
      }
    }
  }

  // move ctor
  SharedCount(SharedCount&& other) noexcept : control_block_(other.control_block_) {
    other.control_block_ = nullptr;
  }

  // 获取 inplace 对象指针
  template <typename T> 
  T* GetInplacePointer() noexcept {
    typedef SpCountedImplPdi<T> ImplType;
    ImplType* point = static_cast<ImplType*>(control_block_);
    return point ? point->get_pointer() : nullptr;
  }

  ~SharedCount() noexcept {
    if (control_block_) {
      control_block_->Release();
    }
  }

  SharedCount& operator=(const SharedCount& other) noexcept {
    if (control_block_ != other.control_block_) {
      SpCountedBase* new_control_block = other.control_block_;
      if (new_control_block) {
        new_control_block->AddRefCopy();
      }
      if (control_block_) {
        control_block_->Release();
      }
      control_block_ = new_control_block;
    }
    return *this;
  }

  // move assign
  SharedCount& operator=(SharedCount&& other) noexcept {
    if (this != &other) {
      SharedCount(std::move(other)).Swap(*this);
    }
    return *this;
  }

  void Swap(SharedCount& other) noexcept {
    SpCountedBase* tmp = control_block_;
    control_block_ = other.control_block_;
    other.control_block_ = tmp;
  }

  int64_t use_count() const noexcept {
    return control_block_ ? control_block_->use_count() : 0;
  }

  bool empty() const noexcept { return control_block_ == nullptr; }

  bool OwnerBefore(const SharedCount& other) const noexcept {
    return control_block_ < other.control_block_;
  }

  //  友元声明
  friend class WeakCount;
  template <typename T>
  friend class my::SharedPtr;
  template <typename T>
  friend class my::WeakPtr;

 private:
  SpCountedBase* control_block_;
};

// 延迟定义
inline WeakCount::WeakCount(const SharedCount& other) noexcept
    : control_block_(other.control_block_) {
  if (control_block_) {
    control_block_->WeakAddRef();
  }
}

inline WeakCount& WeakCount::operator=(const SharedCount& other) noexcept {
  if (control_block_ != other.control_block_) {
    SpCountedBase* new_control_block = other.control_block_;
    if (new_control_block) {
      new_control_block->WeakAddRef();
    }
    if (control_block_) {
      control_block_->WeakRelease();
    }
    control_block_ = new_control_block;
  }
  return *this;
}

}  // namespace detail
}  // namespace my

#endif  // MY_SHARED_COUNT_HPP_
