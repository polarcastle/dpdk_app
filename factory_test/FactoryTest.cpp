#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>

// 抽象产品
class Product {
public:
    virtual ~Product() {}
    virtual void Use() const = 0;
};

// 具体产品A
class ConcreteProductA : public Product {
public:
    void Use() const override {
        std::cout << "Using ConcreteProductA" << std::endl;
    }
};

// 具体产品B
class ConcreteProductB : public Product {
public:
    void Use() const override {
        std::cout << "Using ConcreteProductB" << std::endl;
    }
};

// 工厂
class Factory {
private:
    using ProductCreator = std::function<Product*()>;
    std::unordered_map<std::string, ProductCreator> creators;

public:
    Factory() {
        creators["A"] = []() -> Product* { return new ConcreteProductA(); };
        creators["B"] = []() -> Product* { return new ConcreteProductB(); };
    }

    Product* CreateProduct(const std::string& type) {
        auto it = creators.find(type);
        if (it != creators.end()) {
            return it->second();
        }
        return nullptr; // 如果没有匹配的类型，返回null
    }
};

int main() {
    // 客户端代码
    Factory factory;

    Product* myProductA = factory.CreateProduct("A");
    if (myProductA != nullptr) {
        myProductA->Use();
        delete myProductA;
    }

    Product* myProductB = factory.CreateProduct("B");
    if (myProductB != nullptr) {
        myProductB->Use();
        delete myProductB;
    }

    return 0;
}
