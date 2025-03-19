cmake_minimum_required(VERSION 3.1)  # CMake 3.1 或更高版本支持 C++11 的设置

# 设置项目名称和版本
project(cbtest VERSION 1.0)

# 设置 C++14 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(CBTest cb_main.cpp)

# 如果有其他源文件，可以继续添加
# add_executable(MyCxx11Executable main.cpp other_file.cpp)

# 如果需要链接其他库，可以在这里添加
# target_link_libraries(MyCxx11Executable some_library)
