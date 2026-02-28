#include "my_make_shared.h"
#include "my_pointer_cast.h"
#include "my_weak_ptr.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <memory>  // for std::shared_ptr
#include <thread>

// ============================================================================
// Benchmark 工具
// ============================================================================

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ============================================================================
// 测试用类
// ============================================================================

struct SmallObject {
    int value;
    SmallObject(int v = 0) : value(v) {}
};

struct MediumObject {
    int data[16];  // 64 bytes
    MediumObject() {
        for (int i = 0; i < 16; ++i) data[i] = i;
    }
};

struct LargeObject {
    char data[1024];  // 1KB
    LargeObject() { data[0] = 'x'; }
};

class Base {
public:
    virtual ~Base() {}
    virtual int get_value() const { return 0; }
};

class Derived : public Base {
public:
    int get_value() const override { return 42; }
};

// ============================================================================
// Benchmark 函数
// ============================================================================

void benchmark_creation_and_destruction() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 1: 创建与销毁性能对比              ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int ITERATIONS = 1000000;
    
    std::cout << "\n[小对象 - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::shared_ptr (new)
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            std::shared_ptr<SmallObject> sp(new SmallObject(i));
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr (new):       " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // std::make_shared
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto sp = std::make_shared<SmallObject>(i);
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "std::make_shared:            " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::shared_ptr (new)
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            my::SharedPtr<SmallObject> sp(new SmallObject(i));
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr (new):        " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::make_shared
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto sp = my::make_shared<SmallObject>(i);
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "my::make_shared:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    std::cout << "\n[中等对象 - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::shared_ptr (new)
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            std::shared_ptr<MediumObject> sp(new MediumObject);
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr (new):       " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // std::make_shared
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto sp = std::make_shared<MediumObject>();
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "std::make_shared:            " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::shared_ptr (new)
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            my::SharedPtr<MediumObject> sp(new MediumObject);
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr (new):        " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::make_shared
    {
        Timer timer;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto sp = my::make_shared<MediumObject>();
            (void)sp;
        }
        double elapsed = timer.elapsed_ms();
        std::cout << "my::make_shared:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
}

void benchmark_copy_operations() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 2: 拷贝操作性能对比                 ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int ITERATIONS = 10000000;
    
    std::cout << "\n[拷贝构造 - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::shared_ptr
    {
        std::shared_ptr<int> source(new int(42));
        Timer timer;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            std::shared_ptr<int> copy = source;
            (void)copy;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (" << std::fixed << std::setprecision(1) 
                  << (elapsed / ITERATIONS * 1000000) << " ns/次)\n";
    }
    
    // my::shared_ptr
    {
        my::SharedPtr<int> source(new int(42));
        Timer timer;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            my::SharedPtr<int> copy = source;
            (void)copy;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr:              " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (" << std::fixed << std::setprecision(1) 
                  << (elapsed / ITERATIONS * 1000000) << " ns/次)\n";
    }
}

void benchmark_access_performance() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 3: 对象访问性能对比                 ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int ITERATIONS = 100000000;
    
    std::cout << "\n[解引用操作 - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // 原始指针(基准)
    {
        int* raw = new int(42);
        Timer timer;
        
        long sum = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            sum += *raw;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "原始指针 (baseline):         " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  sum=" << sum << "\n";
        delete raw;
    }
    
    // std::shared_ptr
    {
        std::shared_ptr<int> sp(new int(42));
        Timer timer;
        
        long sum = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            sum += *sp;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  sum=" << sum << "\n";
    }
    
    // my::shared_ptr
    {
        my::SharedPtr<int> sp(new int(42));
        Timer timer;
        
        long sum = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            sum += *sp;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr:              " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  sum=" << sum << "\n";
    }
}

void benchmark_weak_ptr() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 4: weak_ptr 性能对比                ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int ITERATIONS = 1000000;
    
    std::cout << "\n[weak_ptr.lock() - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::weak_ptr
    {
        auto sp = std::make_shared<int>(42);
        std::weak_ptr<int> wp = sp;
        
        Timer timer;
        
        int success = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            if (auto locked = wp.lock()) {
                success++;
            }
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::weak_ptr:               " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (成功: " << success << ")\n";
    }
    
    // my::weak_ptr
    {
        auto sp = my::make_shared<int>(42);
        my::WeakPtr<int> wp = sp;
        
        Timer timer;
        
        int success = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            if (auto locked = wp.lock()) {
                success++;
            }
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::weak_ptr:                " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (成功: " << success << ")\n";
    }
}

void benchmark_pointer_cast() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 5: 类型转换性能对比                 ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int ITERATIONS = 5000000;
    
    std::cout << "\n[static_pointer_cast - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::static_pointer_cast
    {
        auto derived = std::make_shared<Derived>();
        Timer timer;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            auto base = std::static_pointer_cast<Base>(derived);
            (void)base;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::static_pointer_cast:    " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::static_pointer_cast
    {
        auto derived = my::make_shared<Derived>();
        Timer timer;
        
        for (int i = 0; i < ITERATIONS; ++i) {
            auto base = my::static_pointer_cast<Base>(derived);
            (void)base;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::static_pointer_cast:     " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    std::cout << "\n[dynamic_pointer_cast (成功) - " << ITERATIONS << " 次迭代]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::dynamic_pointer_cast
    {
        std::shared_ptr<Base> base = std::make_shared<Derived>();
        Timer timer;
        
        int success = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto derived = std::dynamic_pointer_cast<Derived>(base);
            if (derived) success++;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::dynamic_pointer_cast:   " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (成功: " << success << ")\n";
    }
    
    // my::dynamic_pointer_cast
    {
        my::SharedPtr<Base> base = my::make_shared<Derived>();
        Timer timer;
        
        int success = 0;
        for (int i = 0; i < ITERATIONS; ++i) {
            auto derived = my::dynamic_pointer_cast<Derived>(base);
            if (derived) success++;
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::dynamic_pointer_cast:    " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms"
                  << "  (成功: " << success << ")\n";
    }
}

void benchmark_multithreaded() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 6: 多线程性能对比                   ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS_PER_THREAD = 1000000;
    
    std::cout << "\n[" << NUM_THREADS << " 线程并发拷贝 - " 
              << (NUM_THREADS * ITERATIONS_PER_THREAD) << " 次总计]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::shared_ptr
    {
        auto source = std::make_shared<int>(42);
        Timer timer;
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&source]() {
                for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
                    std::shared_ptr<int> copy = source;
                    (void)copy;
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::shared_ptr
    {
        auto source = my::make_shared<int>(42);
        Timer timer;
        
        std::vector<std::thread> threads;
        for (int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&source]() {
                for (int i = 0; i < ITERATIONS_PER_THREAD; ++i) {
                    my::SharedPtr<int> copy = source;
                    (void)copy;
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr:              " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
}

void benchmark_container_usage() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Benchmark 7: 容器中使用性能对比               ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    constexpr int CONTAINER_SIZE = 100000;
    
    std::cout << "\n[std::vector 插入 - " << CONTAINER_SIZE << " 个元素]\n";
    std::cout << std::string(60, '-') << "\n";
    
    // std::shared_ptr
    {
        Timer timer;
        std::vector<std::shared_ptr<int>> vec;
        vec.reserve(CONTAINER_SIZE);
        
        for (int i = 0; i < CONTAINER_SIZE; ++i) {
            vec.push_back(std::make_shared<int>(i));
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "std::shared_ptr:             " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
    
    // my::shared_ptr
    {
        Timer timer;
        std::vector<my::SharedPtr<int>> vec;
        vec.reserve(CONTAINER_SIZE);
        
        for (int i = 0; i < CONTAINER_SIZE; ++i) {
            vec.push_back(my::make_shared<int>(i));
        }
        
        double elapsed = timer.elapsed_ms();
        std::cout << "my::shared_ptr:              " 
                  << std::fixed << std::setprecision(2) << std::setw(8) << elapsed << " ms\n";
    }
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║  my::shared_ptr vs std::shared_ptr            ║\n";
    std::cout << "║  性能对比测试                                  ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    std::cout << "\n 提示: 请确保以 Release 模式编译(-O3 或 /O2)\n";
    std::cout << "   编译器: " << 
#ifdef __GNUC__
        "GCC " << __GNUC__ << "." << __GNUC_MINOR__
#elif defined(_MSC_VER)
        "MSVC " << _MSC_VER
#else
        "Unknown"
#endif
        << "\n";
    
    std::cout << "   优化级别: " <<
#ifdef NDEBUG
        "Release (优化已启用)"
#else
        "Debug ( 警告: 未启用优化!)"
#endif
        << "\n";
    
    benchmark_creation_and_destruction();
    benchmark_copy_operations();
    benchmark_access_performance();
    benchmark_weak_ptr();
    benchmark_pointer_cast();
    benchmark_multithreaded();
    benchmark_container_usage();
    
    
    return 0;
}