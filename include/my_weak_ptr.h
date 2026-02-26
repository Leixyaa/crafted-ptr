// my_WeakPtr.hpp
#ifndef MY_MY_WEAK_PTR_HPP_
#define MY_MY_WEAK_PTR_HPP_

#include <cstddef>
#include <stdint.h>
#include <utility>

#include "shared_count.h"

namespace my {

template <typename T>
class SharedPtr;  // 前向声明

namespace detail {
//  标签类型:表示从 weak_ptr 构造时不抛异常
struct SpNothrowTag;
}  // namespace detail

// ============================================================================
// WeakPtr: 弱引用智能指针
// ============================================================================

template <typename T>
class WeakPtr {
 public:
  using element_type = T;

  // ------------------------------------------------------------------------
  // 构造函数
  // ------------------------------------------------------------------------

  // 默认构造:空 WeakPtr
  WeakPtr() noexcept : ptr_(nullptr), count_() {}

  //  从 SharedPtr 构造
  template <typename Y>
  WeakPtr(const SharedPtr<Y>& other) noexcept
      : ptr_(other.ptr_), count_(other.count_) {
    // count_(other.count_) 会调用 WeakPtr(SharedCount&)
    // 增加 weak_count_
  }

  // 拷贝构造
  WeakPtr(const WeakPtr& other) noexcept : ptr_(other.ptr_), count_(other.count_) {}

  template <typename Y>
  WeakPtr(const WeakPtr<Y>& other) noexcept
      : ptr_(other.ptr_), count_(other.count_) {}

  // 移动构造
  WeakPtr(WeakPtr&& other) noexcept
      : ptr_(other.ptr_), count_(std::move(other.count_)) {
    other.ptr_ = nullptr;
  }

  template <typename Y>
  WeakPtr(WeakPtr<Y>&& other) noexcept
      : ptr_(other.ptr_), count_(std::move(other.count_)) {
    other.ptr_ = nullptr;
  }

  // ------------------------------------------------------------------------
  // 析构函数
  // ------------------------------------------------------------------------

  ~WeakPtr() noexcept {
    // count_ 析构会调用 weak_release()
  }

  // ------------------------------------------------------------------------
  // 赋值运算符
  // ------------------------------------------------------------------------

  WeakPtr& operator=(const WeakPtr& other) noexcept {
    ptr_ = other.ptr_;
    count_ = other.count_;
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=(const WeakPtr<Y>& other) noexcept {
    ptr_ = other.ptr_;
    count_ = other.count_;
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=(const SharedPtr<Y>& other) noexcept {
    ptr_ = other.ptr_;
    count_ = other.count_;  // weak_count = shared_count
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    WeakPtr(std::move(other)).Swap(*this);
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=(WeakPtr<Y>&& other) noexcept {
    WeakPtr(std::move(other)).Swap(*this);
    return *this;
  }

  // ------------------------------------------------------------------------
  // 核心方法
  // ------------------------------------------------------------------------

  //  lock():安全地提升为 SharedPtr
  SharedPtr<T> lock() const noexcept {
    // 使用 shared_count(weak_count&) 构造
    // 内部会调用 add_ref_lock()
    return SharedPtr<T>(*this, detail::SpNothrowTag());
  }

  //  expired():检查对象是否已销毁
  bool expired() const noexcept { return count_.use_count() == 0; }

  // use_count():返回强引用计数
  int64_t use_count() const noexcept { return count_.use_count(); }

  // ------------------------------------------------------------------------
  // 修改器
  // ------------------------------------------------------------------------

  void Reset() noexcept {
    WeakPtr tmp;
    tmp.swap(*this);
  }

  void Swap(WeakPtr& other) noexcept {
    T* tmp_ptr = ptr_;
    ptr_ = other.ptr_;
    other.ptr_ = tmp_ptr;

    count_.swap(other.count_);
  }

  void swap(WeakPtr& other) noexcept { Swap(other); }

 private:
  T* ptr_;                 // 对象指针(可能已失效)
  detail::WeakCount count_;  // 弱引用计数

  template <typename Y>
  friend class WeakPtr;
  template <typename Y>
  friend class SharedPtr;
};

// ============================================================================
// 比较运算符
// ============================================================================

template <typename T, typename U>
bool operator<(const WeakPtr<T>& a, const WeakPtr<U>& b) noexcept {
  return a.count_.OwnerBefore(b.count_);  // 比较控制块地址
}

}  // namespace my

#endif  // MY_MY_WEAK_PTR_HPP_
