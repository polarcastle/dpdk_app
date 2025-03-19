#include <boost/circular_buffer.hpp>
#include <iostream>

int main() {
    boost::circular_buffer<int> cb(3); // 创建一个容量为3的循环缓冲区

    cb.push_back(1); // 添加元素
    cb.push_back(2);
    cb.push_back(3);

    // 此时缓冲区满，下一个元素将覆盖最旧的元素
    cb.push_back(4); // 这将导致1被覆盖

    for (int i : cb) {
        std::cout << i << ' '; // 输出: 2 3 4
    }

    return 0;
}
