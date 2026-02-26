#include "my_shared_ptr.h"
#include "my_weak_ptr.h"
#include <iostream>
#include <cassert>
#include <memory>  // 用于对比 std::SharedPtr
#include <map>
#include <vector>
// ============================================================================
// 案例 1:双向链表节点
// ============================================================================

namespace case1 {

class Node {
public:
    Node(int value) : value_(value) {
        std::cout << "  [构造] Node(" << value_ << ")\n";
        ++node_count;
    }
    
    ~Node() {
        std::cout << "  [析构] ~Node(" << value_ << ")\n";
        --node_count;
    }
    
    int value_;
    my::SharedPtr<Node> next;  // 强引用:拥有下一个节点
    my::WeakPtr<Node> prev;    //  弱引用:不拥有前一个节点
    
    static int node_count;
};

int Node::node_count = 0;

void test_bidirectional_list() {
    std::cout << "\n========== 案例 1:双向链表 ==========\n";
    
    {
        my::SharedPtr<Node> n1(new Node(1));
        my::SharedPtr<Node> n2(new Node(2));
        my::SharedPtr<Node> n3(new Node(3));
        
        // 建立链接
        n1->next = n2;
        n2->prev = n1;  //  使用 WeakPtr
        
        n2->next = n3;
        n3->prev = n2;  //  使用 WeakPtr
        
        std::cout << "链表建立完成:\n";
        std::cout << "  n1 use_count: " << n1.use_count() << "\n";
        std::cout << "  n2 use_count: " << n2.use_count() << "\n";
        std::cout << "  n3 use_count: " << n3.use_count() << "\n";
        
        // 从 n2 访问前一个节点
        if (my::SharedPtr<Node> prev = n2->prev.lock()) {
            std::cout << "n2 的前一个节点: " << prev->value_ << "\n";
            assert(prev->value_ == 1);
        }
        
        std::cout << "作用域结束,节点将被释放...\n";
    }
    
    std::cout << "作用域结束后,活跃节点数: " << Node::node_count << "\n";
    assert(Node::node_count == 0);  //  全部释放
    
    std::cout << " 案例通过:双向链表无泄漏\n";
}

} // namespace case1

// ============================================================================
// 案例 2:父子关系
// ============================================================================

namespace case2 {

class Child;

class Parent {
public:
    Parent(int id) : id_(id) {
        std::cout << "  [构造] Parent(" << id_ << ")\n";
        ++parent_count;
    }
    
    ~Parent() {
        std::cout << "  [析构] ~Parent(" << id_ << ")\n";
        --parent_count;
    }
    
    void add_child(my::SharedPtr<Child> child) {
        children_.push_back(child);
    }
    
    int id_;
    std::vector<my::SharedPtr<Child>> children_;  // 强引用:拥有子对象
    
    static int parent_count;
};

int Parent::parent_count = 0;

class Child {
public:
    Child(int id) : id_(id) {
        std::cout << "  [构造] Child(" << id_ << ")\n";
        ++child_count;
    }
    
    ~Child() {
        std::cout << "  [析构] ~Child(" << id_ << ")\n";
        --child_count;
    }
    
    void set_parent(my::SharedPtr<Parent> p) {
        parent_ = p;  //  使用 WeakPtr
    }
    
    my::SharedPtr<Parent> get_parent() const {
        return parent_.lock();
    }
    
    int id_;
    my::WeakPtr<Parent> parent_;  //  弱引用:不拥有父对象
    
    static int child_count;
};

int Child::child_count = 0;

void test_parent_child() {
    std::cout << "\n========== 案例 2:父子关系 ==========\n";
    
    {
        my::SharedPtr<Parent> parent(new Parent(100));
        
        my::SharedPtr<Child> child1(new Child(1));
        my::SharedPtr<Child> child2(new Child(2));
        
        // 建立关系
        parent->add_child(child1);
        parent->add_child(child2);
        
        child1->set_parent(parent);
        child2->set_parent(parent);
        
        std::cout << "关系建立完成:\n";
        std::cout << "  parent use_count: " << parent.use_count() << "\n";
        std::cout << "  child1 use_count: " << child1.use_count() << "\n";
        
        // 从子对象访问父对象
        if (my::SharedPtr<Parent> p = child1->get_parent()) {
            std::cout << "child1 的父对象: " << p->id_ << "\n";
            assert(p->id_ == 100);
        }
        
        std::cout << "作用域结束...\n";
    }
    
    std::cout << "活跃对象数:\n";
    std::cout << "  Parent: " << Parent::parent_count << "\n";
    std::cout << "  Child: " << Child::child_count << "\n";
    
    assert(Parent::parent_count == 0);
    assert(Child::child_count == 0);
    
    std::cout << " 案例通过:父子关系无泄漏\n";
}

} // namespace case2

// ============================================================================
// 案例 3:缓存系统
// ============================================================================

namespace case3 {

class Resource {
public:
    Resource(int id) : id_(id), data_(new char[1024]) {
        std::cout << "  [构造] Resource(" << id_ << ") - 分配 1KB 内存\n";
        ++resource_count;
    }
    
    ~Resource() {
        std::cout << "  [析构] ~Resource(" << id_ << ") - 释放内存\n";
        delete[] data_;
        --resource_count;
    }
    
    int id_;
    char* data_;
    
    static int resource_count;
};

int Resource::resource_count = 0;

class ResourceCache {
public:
    my::SharedPtr<Resource> get(int id) {
        // 先检查缓存
        auto it = cache_.find(id);
        if (it != cache_.end()) {
            // 尝试提升为 SharedPtr
            my::SharedPtr<Resource> sp = it->second.lock();
            if (sp) {
                std::cout << "  从缓存命中 Resource(" << id << ")\n";
                return sp;
            } else {
                std::cout << "  缓存失效,重新加载 Resource(" << id << ")\n";
                cache_.erase(it);
            }
        }
        
        // 创建新资源
        my::SharedPtr<Resource> sp(new Resource(id));
        cache_[id] = sp;  //  存储 WeakPtr
        return sp;
    }
    
    void show_cache_status() {
        std::cout << "缓存状态:\n";
        for (auto it = cache_.begin(); it != cache_.end(); ) {
            if (it->second.expired()) {
                std::cout << "  Resource(" << it->first << "): 已过期\n";
                it = cache_.erase(it);
            } else {
                std::cout << "  Resource(" << it->first << "): 有效\n";
                ++it;
            }
        }
    }
    
private:
    std::map<int, my::WeakPtr<Resource>> cache_;  //  WeakPtr 缓存
};

void test_resource_cache() {
    std::cout << "\n========== 案例 3:资源缓存 ==========\n";
    
    ResourceCache cache;
    
    {
        my::SharedPtr<Resource> r1 = cache.get(1);
        my::SharedPtr<Resource> r2 = cache.get(2);
        
        std::cout << "\n第一次获取:\n";
        cache.show_cache_status();
        
        // 再次获取(命中缓存)
        my::SharedPtr<Resource> r1_again = cache.get(1);
        assert(r1.get() == r1_again.get());  //  同一对象
        
        // r2 释放
        r2.Reset();
        std::cout << "\nr2 释放后:\n";
        cache.show_cache_status();
    }
    
    std::cout << "\n所有外部引用释放后:\n";
    cache.show_cache_status();
    
    std::cout << "活跃资源数: " << Resource::resource_count << "\n";
    assert(Resource::resource_count == 0);
    
    std::cout << " 案例通过:资源缓存无泄漏\n";
}

} // namespace case3

// ============================================================================
// 对比测试:使用 SharedPtr 的循环引用问题
// ============================================================================

void demonstrate_cycle_leak() {
    std::cout << "\n========== 演示:SharedPtr 循环引用泄漏 ==========\n";
    
    struct BadNode {
        BadNode(int v) : value(v) {
            std::cout << "  [构造] BadNode(" << value << ")\n";
        }
        ~BadNode() {
            std::cout << "  [析构] ~BadNode(" << value << ")\n";
        }
        
        int value;
        my::SharedPtr<BadNode> next;
        my::SharedPtr<BadNode> prev;  //  错误:使用 SharedPtr
    };
    
    std::cout << "创建循环引用:\n";
    {
        my::SharedPtr<BadNode> n1(new BadNode(1));
        my::SharedPtr<BadNode> n2(new BadNode(2));
        
        n1->next = n2;
        n2->prev = n1;  //  循环!
        
        std::cout << "  n1 use_count: " << n1.use_count() << " (应该是 1, 实际 " << n1.use_count() << ")\n";
        std::cout << "  n2 use_count: " << n2.use_count() << " (应该是 1, 实际 " << n2.use_count() << ")\n";
        
        std::cout << "作用域结束...(注意:析构函数不会被调用!)\n";
    }
    
    std::cout << " 内存泄漏!BadNode 对象永远不会被释放\n";
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║   Day 5: 循环引用实战案例                  ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n";
    
    case1::test_bidirectional_list();
    case2::test_parent_child();
    case3::test_resource_cache();
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    demonstrate_cycle_leak();
    std::cout << std::string(50, '=') << "\n";
    
    return 0;
}