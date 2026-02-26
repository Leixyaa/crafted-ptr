#ifndef MY_SP_COUNTED_BASE_HPP_
#define MY_SP_COUNTED_BASE_HPP_

#include <atomic>
#include <stdint.h>

namespace my {
namespace detail {

// ============================================================================
// 辅助函数:原子操作的封装(参考 Boost 实现)
// ============================================================================

// 原子递增
inline void AtomicIncrement(std::atomic<int64_t>* counter) noexcept {
  counter->fetch_add(1, std::memory_order_relaxed);
  // relaxed:只需保证原子性,不需要内存同步
}

// 原子递减 返回变化前的值
inline int64_t AtomicDecrement(std::atomic<int64_t>* counter) noexcept {
  return counter->fetch_sub(1, std::memory_order_acq_rel);
  // acq_rel:
  // - release:当前线程的写操作对其他线程可见
  // - acquire:看到其他线程的所有写操作
  // 这样才能安全删除对象!
}

// 条件递增:如果当前值非 0,则 +1
// 返回递增前的值(如果是 0 则返回 0)
inline int64_t AtomicConditionalIncrement(
    std::atomic<int64_t>* counter) noexcept {
  // 等价于:
  // long r = *pw;
  // if (r != 0) ++*pw;
  // return r;

  int64_t expected_count = counter->load(std::memory_order_relaxed);
  for (;;) {
    if (expected_count == 0) return expected_count; // 已经是0则无法增加

    if (counter->compare_exchange_weak(expected_count, expected_count + 1,
                                       std::memory_order_relaxed,
                                       std::memory_order_relaxed)) {
      return expected_count; // 返回旧值
    }
  }
}



// ============================================================================
// SpCountedBase: 引用计数控制块的抽象基类
// ============================================================================
// 这是 Boost shared_ptr 的核心设计：
// 通过虚函数 Dispose() 实现多态删除，从而支持自定义删除器和类型擦除。

class SpCountedBase {
 public:
  SpCountedBase() : use_count_(1), weak_count_(1) {}

  virtual ~SpCountedBase() noexcept = default;

  // 释放被管理对象 (use_count_ 变为 0 时调用)
  virtual void Dispose() noexcept = 0;

  // 释放控制块自身 (weak_count_ 变为 0 时调用)
  virtual void Destroy() noexcept { delete this; }

  // 强引用计数操作
  void AddRefCopy() noexcept {
    AtomicIncrement(&use_count_);
  }
  
  // 条件增加引用计数(用于 weak_ptr::lock)
  bool AddRefLock() noexcept {
    return AtomicConditionalIncrement(&use_count_) != 0;
  }

  void Release() noexcept {
    if (AtomicDecrement(&use_count_) == 1) {
      Dispose();
      WeakRelease();
    }
  }

  // 弱引用计数操作
  void WeakAddRef() noexcept {
    AtomicIncrement(&weak_count_);
  }

  void WeakRelease() noexcept {
    if (AtomicDecrement(&weak_count_) == 1) {
      Destroy();
    }
  }

  // 观察器
  int64_t use_count() const noexcept { 
    return use_count_.load(std::memory_order_acquire);
    // acquire:确保看到最新的值
  }

 protected:
  std::atomic<int64_t> use_count_;   // 强引用计数 (shared_ptr)
  std::atomic<int64_t> weak_count_;  // 弱引用计数 (weak_ptr + “强引用存在”)

 private:
  SpCountedBase(const SpCountedBase&) = delete;
  SpCountedBase& operator=(const SpCountedBase&) = delete;
};

}  // namespace detail
}  // namespace my

#endif  // MY_SP_COUNTED_BASE_HPP_
