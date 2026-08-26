cmake_minimum_required(VERSION 3.10)
project(dpdk_traffic_copy LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 pkg-config
find_package(PkgConfig REQUIRED)

# 查找 DPDK
pkg_check_modules(DPDK REQUIRED libdpdk)

# 添加可执行文件
add_executable(main main.cpp)
add_executable(subprocess subprocess.cpp)

# 为每个目标设置包含目录和链接库
foreach(target main subprocess)
    target_include_directories(${target} PRIVATE ${DPDK_INCLUDE_DIRS})
    target_compile_options(${target} PRIVATE ${DPDK_CFLAGS_OTHER} -Wall -Wextra -O3)
    target_link_libraries(${target} PRIVATE ${DPDK_LIBRARIES} pthread)
endforeach()

# 可选：安装规则
install(TARGETS main subprocess
    RUNTIME DESTINATION bin
)
