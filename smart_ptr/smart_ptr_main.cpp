#include <iostream>
#include <memory>
#include <string>

// 一个用于演示生命周期的简单类
class Resource {
public:
    explicit Resource(const std::string& name) : name_(name) {
        std::cout << "[Resource] " << name_ << " 构造" << std::endl;
    }
    ~Resource() {
        std::cout << "[Resource] " << name_ << " 析构" << std::endl;
    }
    void use() const {
        std::cout << "[Resource] " << name_ << " 被使用" << std::endl;
    }
private:
    std::string name_;
};

// 1. unique_ptr：独占所有权，支持移动语义
static void demo_unique_ptr() {
    std::cout << "\n=== unique_ptr 演示 ===" << std::endl;

    std::unique_ptr<Resource> p1(new Resource("unique_res"));
    p1->use();

    // 所有权转移：p1 变为空，p2 接管
    std::unique_ptr<Resource> p2 = std::move(p1);
    std::cout << "移动后 p1 是否为空: " << (p1 == nullptr ? "是" : "否") << std::endl;
    p2->use();

    // C++14 推荐写法：std::make_unique（本示例使用 C++11，故用 new 演示）
} // p2 离开作用域，自动析构

// 2. shared_ptr：共享所有权，引用计数
static void demo_shared_ptr() {
    std::cout << "\n=== shared_ptr 演示 ===" << std::endl;

    std::shared_ptr<Resource> p1 = std::make_shared<Resource>("shared_res");
    std::cout << "引用计数: " << p1.use_count() << std::endl;

    {
        std::shared_ptr<Resource> p2 = p1;  // 共享所有权
        std::cout << "拷贝后引用计数: " << p1.use_count() << std::endl;
        p2->use();
    } // p2 析构，计数减 1

    std::cout << "p2 析构后引用计数: " << p1.use_count() << std::endl;
} // p1 析构，计数归零，资源释放

// 3. weak_ptr：配合 shared_ptr 使用，打破循环引用
struct Node {
    std::string name;
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;   // 若 prev 也用 shared_ptr，双向链表将循环引用无法释放

    explicit Node(const std::string& n) : name(n) {
        std::cout << "[Node] " << name << " 构造" << std::endl;
    }
    ~Node() {
        std::cout << "[Node] " << name << " 析构" << std::endl;
    }
};

static void demo_weak_ptr() {
    std::cout << "\n=== weak_ptr 演示 ===" << std::endl;

    std::shared_ptr<Node> a = std::make_shared<Node>("A");
    std::shared_ptr<Node> b = std::make_shared<Node>("B");

    a->next = b;
    b->prev = a;    // weak_ptr 不增加引用计数

    // weak_ptr 使用前需 lock() 提升为 shared_ptr
    if (std::shared_ptr<Node> prev_of_b = b->prev.lock()) {
        std::cout << "B 的前驱是: " << prev_of_b->name << std::endl;
    }

    std::cout << "a 引用计数: " << a.use_count()
              << ", b 引用计数: " << b.use_count() << std::endl;
} // a、b 正常析构，无循环引用泄漏

int main() {
    std::cout << "=== C++11 智能指针示例 ===" << std::endl;

    demo_unique_ptr();
    demo_shared_ptr();
    demo_weak_ptr();

    std::cout << "\n=== 示例结束 ===" << std::endl;
    return 0;
}
