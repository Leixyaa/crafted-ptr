#ifndef MY_SP_COUNTED_IMPL_HPP_
#define MY_SP_COUNTED_IMPL_HPP_

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

}  // namespace detail
}  // namespace my

#endif  // MY_SP_COUNTED_IMPL_HPP_
