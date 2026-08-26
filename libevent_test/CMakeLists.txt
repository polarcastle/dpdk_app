cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版本
project(libevent_demo VERSION 1.0)

# 设置 C++11 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 查找 libevent2（event 核心库与 pthread 支持库）
find_library(EVENT_LIBRARY NAMES event)
find_library(EVENT_PTHREADS_LIBRARY NAMES event_pthreads)

if(NOT EVENT_LIBRARY)
  message(FATAL_ERROR "libevent (libevent2) 未找到，请先安装：sudo apt install libevent-dev")
endif()

# 添加可执行文件
add_executable(libevent_demo main.cpp)
target_link_libraries(libevent_demo PRIVATE ${EVENT_LIBRARY})

if(EVENT_PTHREADS_LIBRARY)
  target_link_libraries(libevent_demo PRIVATE ${EVENT_PTHREADS_LIBRARY})
endif()
