// my_pointer_cast.h
#ifndef MY_POINTER_CAST_HPP
#define MY_POINTER_CAST_HPP

#include "my_shared_ptr.h"

namespace my {

// ============================================================================
// static_pointer_cast: 静态类型转换(编译期)
// ============================================================================

template <typename T, typename U>
SharedPtr<T> static_pointer_cast(const SharedPtr<U>& other) noexcept {
    // 使用别名构造函数
    // - 转换指针类型:static_cast<T*>(r.get())
    // - 共享控制块:r.pn_
    
    T* p = static_cast<T*>(other.get());
    return SharedPtr<T>(other, p);
}


// ============================================================================
// dynamic_pointer_cast: 动态类型转换(运行期检查)
// ============================================================================

template<typename T, typename U>
SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U>& other) noexcept {
    // 尝试动态类型转换
    T* p = dynamic_cast<T*>(other.get());
    
    if (p) {
        // 转换成功:使用别名构造
        return SharedPtr<T>(other, p);
    } else {
        // 转换失败:返回空指针
        return SharedPtr<T>();
    }
}

// ============================================================================
// const_pointer_cast: 移除 const
// ============================================================================

template<typename T, typename U>
SharedPtr<T> const_pointer_cast(const SharedPtr<U>& other) noexcept {
    // 移除 const 限定符
    T* p = const_cast<T*>(other.get());
    return SharedPtr<T>(other, p);
}

} // namespace my
#endif // MY_POINTER_CAST_HPP