#include <iostream>

// 模板函数，用于找出两个值中的最大值
template <typename T>
T max(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    // 使用模板函数计算不同类型的数据
    int i1 = 5, i2 = 10;
    double d1 = 3.5, d2 = 2.5;
    char c1 = 'a', c2 = 'z';

    // 输出结果
    std::cout << "Max of " << i1 << " and " << i2 << " is " << max(i1, i2) << std::endl;
    std::cout << "Max of " << d1 << " and " << d2 << " is " << max(d1, d2) << std::endl;
    std::cout << "Max of " << c1 << " and " << c2 << " is " << max(c1, c2) << std::endl;

    return 0;
}
