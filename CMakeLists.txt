cmake_minimum_required(VERSION 3.10)

project(dpdk_app VERSION 1.0 LANGUAGES C CXX)

# Default C++ standard (can be overridden by user)
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to use")
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DPDK_APPS "Build DPDK-related executables (may require pkg-config libdpdk)" ON)
option(BUILD_SUBPROJECTS "Build subprojects (tests/examples)" ON)

# Try to find DPDK via pkg-config (optional)
find_package(PkgConfig QUIET)
if(BUILD_DPDK_APPS AND PKG_CONFIG_FOUND)
  pkg_check_modules(DPDK libdpdk QUIET)
  if(DPDK_FOUND)
    message(STATUS "Found DPDK via pkg-config: ${DPDK_VERSION}")
    include_directories(${DPDK_INCLUDE_DIRS})
    link_directories(${DPDK_LIBRARY_DIRS})
    add_compile_options(${DPDK_CFLAGS_OTHER})
  else()
    message(STATUS "DPDK pkg-config module 'libdpdk' not found. You can still build, but will need to provide include/link paths manually.")
  endif()
endif()

# Root DPDK main (optional)
if(BUILD_DPDK_APPS)
  if(EXISTS "${CMAKE_SOURCE_DIR}/main.cpp")
    add_executable(dpdk_main main.cpp)
    if(TARGET dpdk_main AND DEFINED DPDK_LIBRARIES)
      target_include_directories(dpdk_main PRIVATE ${DPDK_INCLUDE_DIRS})
      target_link_libraries(dpdk_main PRIVATE ${DPDK_LIBRARIES})
    endif()
  endif()

  # dpdk_copy: separate primary and secondary subprocess
  if(EXISTS "${CMAKE_SOURCE_DIR}/dpdk_copy/main.cpp")
    add_executable(dpdk_copy_primary dpdk_copy/main.cpp)
    if(DEFINED DPDK_LIBRARIES)
      target_include_directories(dpdk_copy_primary PRIVATE ${DPDK_INCLUDE_DIRS})
      target_link_libraries(dpdk_copy_primary PRIVATE ${DPDK_LIBRARIES})
    endif()
  endif()
  if(EXISTS "${CMAKE_SOURCE_DIR}/dpdk_copy/subprocess.cpp")
    add_executable(dpdk_copy_subprocess dpdk_copy/subprocess.cpp)
    if(DEFINED DPDK_LIBRARIES)
      target_include_directories(dpdk_copy_subprocess PRIVATE ${DPDK_INCLUDE_DIRS})
      target_link_libraries(dpdk_copy_subprocess PRIVATE ${DPDK_LIBRARIES})
    endif()
  endif()
endif()

# Add existing subprojects (tests/examples)
if(BUILD_SUBPROJECTS)
  if(EXISTS "${CMAKE_SOURCE_DIR}/circular_buffer_test/CMakeLists.txt")
    add_subdirectory(circular_buffer_test)
  endif()
  if(EXISTS "${CMAKE_SOURCE_DIR}/factory_test/CMakeLists.txt")
    add_subdirectory(factory_test)
  endif()
  if(EXISTS "${CMAKE_SOURCE_DIR}/template_test/CMakeLists.txt")
    add_subdirectory(template_test)
  endif()
endif()

# Helpful messages
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build DPDK apps: ${BUILD_DPDK_APPS}")
message(STATUS "Build subprojects: ${BUILD_SUBPROJECTS}")

# Quick build instructions printed as a summary target
add_custom_target(show_build_instructions
  COMMAND ${CMAKE_COMMAND} -E echo "To configure: cmake -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR} -G \"Ninja\" -DCMAKE_BUILD_TYPE=Release"
  COMMAND ${CMAKE_COMMAND} -E echo "To build: cmake --build ${CMAKE_BINARY_DIR} -- -j$(nproc)"
  COMMENT "Print quick build instructions"
)
