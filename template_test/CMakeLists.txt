cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(TemplateProgrammingProject VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(MyTemplateProgram TemplateClassTest.cpp)

# 如果你有其他源文件，继续添加它们
# add_library(MyTemplateLib src/TemplateClass.cpp)

# 添加包含目录
include_directories(include)

# 添加链接目录（如果需要链接其他库）
# link_directories(lib)

# 链接库（如果需要）
# target_link_libraries(MyTemplateProgram MyTemplateLib)

# 安装规则（如果需要）
# install(TARGETS MyTemplateProgram DESTINATION bin)
# install(FILES include/TemplateClass.h DESTINATION include)
