// my_make_shared.h
#ifndef MY_MAKE_SHARED_H
#define MY_MAKE_SHARED_H

#include <utility>  // for forward

#include "my_shared_ptr.h"

namespace my {

// ============================================================================
// make_shared: 工厂函数,单次内存分配
// ============================================================================

template <typename T, typename... Args>
SharedPtr<T> make_shared(Args&&... args) {
  // 明确调用 inplace 构造函数
  // 第一个参数是 sp_inplace_tag<T>，确保匹配正确的构造函数
  detail::SharedCount control_block(
    detail::sp_inplace_tag<T>{}, 
    std::forward<Args>(args)...
  );

  return SharedPtr<T>(detail::sp_inplace_tag<T>{}, control_block);
}

}; // namespace my




#endif // MY_MAKE_SHARED_H