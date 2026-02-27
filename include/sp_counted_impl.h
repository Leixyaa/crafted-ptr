#ifndef MY_SP_COUNTED_IMPL_HPP_
#define MY_SP_COUNTED_IMPL_HPP_

#include <new>         // for placement new
#include <type_traits>  //  for aligned_storage
#include <utility>      //  for forward

#include "sp_counted_base.h"

namespace my {
namespace detail {

// ============================================================================
// CheckedDelete: 确保类型完整的 delete (思路来自 Boost)
// ============================================================================

template <typename T>
inline void CheckedDelete(T* ptr) noexcept {
  typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
  (void)sizeof(type_must_be_complete);
  delete ptr;
}

template <typename T>
inline void CheckedArrayDelete(T* ptr) noexcept {
  typedef char type_must_be_complete[sizeof(T) ? 1 : -1];
  (void)sizeof(type_must_be_complete);
  delete[] ptr;
}

// ============================================================================
// SpCountedImplPointer: 默认删除器控制块
// ============================================================================

template <typename T>
class SpCountedImplPointer : public SpCountedBase {
 private:
  T* ptr_;

  SpCountedImplPointer(const SpCountedImplPointer&) = delete;
  SpCountedImplPointer& operator=(const SpCountedImplPointer&) = delete;

 public:
  explicit SpCountedImplPointer(T* ptr) noexcept : SpCountedBase(), ptr_(ptr) {}

  void Dispose() noexcept override { delete ptr_; }
};

// ============================================================================
// SpCountedImplPointerDeleter: 自定义删除器控制块
// ============================================================================
// 模板参数：
//   P - 指针类型 (T* / FILE* / void* 等)
//   D - 删除器类型 (函数指针 / 函数对象 / lambda 等)

template <typename P, typename D>
class SpCountedImplPointerDeleter : public SpCountedBase {
 private:
  P ptr_;
  D deleter_;

  SpCountedImplPointerDeleter(const SpCountedImplPointerDeleter&) = delete;
  SpCountedImplPointerDeleter& operator=(
      const SpCountedImplPointerDeleter&) = delete;

 public:
  SpCountedImplPointerDeleter(P ptr, D deleter)
      : ptr_(ptr), deleter_(deleter) {}

  explicit SpCountedImplPointerDeleter(P ptr) : ptr_(ptr), deleter_() {}

  void Dispose() noexcept override { deleter_(ptr_); }
};

// ============================================================================
// SpCountedImplPdi: Inplace 存储实现(第 6 天新增!)
// ============================================================================
// 用于 make_shared:对象内联存储在控制块中
// "pdi" = pointer + deleter + inplace

template <typename T> 
class SpCountedImplPdi : public SpCountedBase {
 private:
  //  使用 aligned_storage 存储对象
  // 这是原始内存,尚未构造对象
  typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;

  SpCountedImplPdi(const SpCountedImplPdi&) = delete;
  SpCountedImplPdi& operator= (const SpCountedImplPdi&) = delete;

 public:
  // ------------------------------------------------------------------------
  // 构造函数:使用 placement new 构造对象
  // ------------------------------------------------------------------------
  
  template<typename... Args>
  explicit SpCountedImplPdi(Args&&... args) {
    ::new(static_cast<void*>(&storage_)) T(std::forward<Args>(args)...);
    // 注意:
    // 1. ::new 是 placement new
    // 2. static_cast<void*> 确保正确的地址
    // 3. Args&& + std::forward 完美转发参数
  }
  
  // 获取对象指针
  T* GetPoint() noexcept {
    // 将 storage_ 的地址转换为 T*
    return reinterpret_cast<T*>(&storage_);
  }

  // 兼容 SharedCount::GetInplacePointer() 的命名
  T* get_pointer() noexcept { return GetPoint(); }

  // ------------------------------------------------------------------------
  // 实现虚函数
  // ------------------------------------------------------------------------
    
  // 释放对象:只调用析构函数,不释放内存
  void Dispose() noexcept override {
    // 显式调用析构函数
    GetPoint()-> ~T();
    // storage_ 的内存在 destroy() 时才释放
  }

  // destroy() 使用基类默认实现:delete this

};


}  // namespace detail
}  // namespace my

#endif  // MY_SP_COUNTED_IMPL_HPP_
