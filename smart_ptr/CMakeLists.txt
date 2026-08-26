cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(SmartPtrDemo VERSION 1.0)

# 设置 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(SmartPtrDemo smart_ptr_main.cpp)
