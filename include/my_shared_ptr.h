#ifndef MY_MY_SHARED_PTR_HPP_
#define MY_MY_SHARED_PTR_HPP_

#include <cstddef>
#include <stdint.h>
#include <type_traits>
#include <utility>
#include <functional>

#include "shared_count.h"

namespace my {

template <typename T>
class WeakPtr;  //  前向声明

namespace detail {
//  标签类型:表示从 weak_ptr 构造时不抛异常
struct SpNothrowTag {};
}  // namespace detail

// ============================================================================
// SharedPtr: 基于控制块的 shared_ptr 
// ============================================================================

template <typename T>
class SharedPtr {
 public:
  using element_type = T;

  // ------------------------------------------------------------------------
  // 构造函数
  // ------------------------------------------------------------------------

  SharedPtr() noexcept : ptr_(nullptr), count_() {}

  SharedPtr(std::nullptr_t) noexcept : ptr_(nullptr), count_() {}

  template <typename Y>
  explicit SharedPtr(Y* ptr) : ptr_(ptr), count_(ptr) {}

  template <typename Y, typename D>
  explicit SharedPtr(Y* ptr, D deleter) 
      : ptr_(ptr), count_(detail::sp_deleter_tag{}, ptr, deleter) {}

  SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
    // std::cout << "11" << std::endl;
  }

  template <typename Y>
  SharedPtr(const SharedPtr<Y>& other) noexcept
      : ptr_(other.ptr_), count_(other.count_) {
    // std::cout << "22" << std::endl;
  }

  //  从 weak_ptr 构造(用于 lock())
  template <typename Y>
  SharedPtr(const WeakPtr<Y>& other, detail::SpNothrowTag) noexcept
      : ptr_(nullptr), count_(other.count_) {
    // pn_(r.pn_) 调用 shared_count(weak_count&)
    // 如果 add_ref_lock() 成功,pn_ 有效
    // 如果失败,pn_ 为空

    if (!count_.empty()) {
      ptr_ = other.ptr_;  //  只有在成功时才设置指针
    }
  }

  // 从 inplace 控制块构造(用于 make_shared)
  template <typename Y>
  SharedPtr(detail::sp_inplace_tag<Y>, detail::SharedCount const& control_block) noexcept
      : ptr_(nullptr), count_(control_block) {
        if (!count_.empty()) {
          ptr_ = const_cast<detail::SharedCount&> (control_block).template GetInplacePointer<Y>();
        } 
      }

  // 别名构造函数
  template <typename Y> 
  SharedPtr(const SharedPtr<Y>& other, element_type* ptr) 
      : ptr_(ptr), count_(other.count_){
        // 存储 p,但共享 r 的控制块
        // 用于类型转换和访问成员
      };

  SharedPtr(SharedPtr&& other) noexcept
      : ptr_(other.ptr_), count_(std::move(other.count_)) {
    other.ptr_ = nullptr;
  }

  ~SharedPtr() = default;

  // ------------------------------------------------------------------------
  // 赋值运算符
  // ------------------------------------------------------------------------

  SharedPtr& operator=(const SharedPtr& other) noexcept {
    ptr_ = other.ptr_;
    count_ = other.count_;
    return *this;
  }

  template <typename Y>
  SharedPtr& operator=(const SharedPtr<Y>& other) noexcept {
    ptr_ = other.ptr_;
    count_ = other.count_;
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this != &other) {
      // 复用你现有 Swap，保证最小改动且安全
      SharedPtr(std::move(other)).Swap(*this);
    }
    return *this;
  }

  // nullptr 赋值
  SharedPtr& operator=(std::nullptr_t) noexcept {
    Reset();
    return *this;
  }

  // ------------------------------------------------------------------------
  // 修改器
  // ------------------------------------------------------------------------

  void Reset() noexcept { SharedPtr().Swap(*this); }

  template <typename Y>
  void Reset(Y* ptr) {
    SharedPtr(ptr).Swap(*this);
  }

  template <typename Y, typename D>
  void Reset(Y* ptr, D deleter) noexcept {
    SharedPtr(ptr, deleter).Swap(*this);
  }

  void Swap(SharedPtr& other) noexcept {
    T* tmp_ptr = ptr_;
    ptr_ = other.ptr_;
    other.ptr_ = tmp_ptr;

    count_.Swap(other.count_);
  }

  // ------------------------------------------------------------------------
  // 观察器
  // ------------------------------------------------------------------------

  // Getter: 允许使用 snake_case
  T* get() const noexcept { return ptr_; }

  template <typename U = T>
  typename std::enable_if<!std::is_void<U>::value, U&>::type operator*() const
      noexcept {
    return *ptr_;
  }

  T* operator->() const noexcept { return ptr_; }

  int64_t use_count() const noexcept { return count_.use_count(); }

  bool unique() const noexcept { return use_count() == 1; }

  explicit operator bool() const noexcept { return ptr_ != nullptr; }

 private:
  T* ptr_;
  detail::SharedCount count_;

  template <typename Y>
  friend class SharedPtr;
  template <typename Y>
  friend class WeakPtr;  //  友元

  // 类型转换需要访问私有成员
  template <typename T1, typename U1>
  friend SharedPtr<T1> static_pointer_cast(const SharedPtr<U1>&) noexcept;
  
  template <typename T1, typename U1>
  friend SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<U1>&) noexcept;

  template <typename T1, typename U1>
  friend SharedPtr<T1> const_pointer_cast(const SharedPtr<U1>&) noexcept;

};

// ============================================================================
// 比较运算符
// ============================================================================

// 相等比较
template <typename T, typename U>
bool operator==(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  return a.get() == b.get();
}

template <typename T, typename U>
bool operator!=(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  return !(a == b);
}

// nullptr 比较
template <typename T>
bool operator==(const SharedPtr<T>& a, std::nullptr_t) noexcept {
  return !a;
}

template <typename T>
bool operator==(std::nullptr_t, const SharedPtr<T>& a) noexcept {
  return !a;
}

template <typename T>
bool operator!=(const SharedPtr<T>& a, std::nullptr_t) noexcept {
  return static_cast<bool>(a);
}

template <typename T>
bool operator!=(std::nullptr_t, const SharedPtr<T>& a) noexcept {
  return static_cast<bool>(a);
}

// 关系比较(用于关联容器)
template <typename T,typename U>
bool operator<(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  typedef typename std::common_type<T*, U*>::type common_type;
  return std::less<common_type>()(a.get(), b.get());
}

template <typename T, typename U>
bool operator>(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  return b < a;
}

template <typename T, typename U>
bool operator<=(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  return !(b < a);
}

template <typename T, typename U>
bool operator>=(const SharedPtr<T>& a, const SharedPtr<U>& b) noexcept {
  return !(a < b);
}

// ============================================================================
// std::swap 特化
// ============================================================================

template <typename T>
void swap(SharedPtr<T>& a, SharedPtr<T>& b) noexcept {
  a.Swap(b);
}

}  // namespace my

// ============================================================================
// std::swap 特化(放在 std 命名空间)
// ============================================================================

namespace std {
  template <typename T>
  void swap(my::SharedPtr<T>& a, my::SharedPtr<T>& b) noexcept {
    a.Swap(b);
  }
}

#endif  // MY_MY_SHARED_PTR_HPP_
