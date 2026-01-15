cmake_minimum_required(VERSION 3.10)

# Tests CMake: find or fallback to third_party googletest
add_executable(test_skiplist test_skiplist.cpp test_memtable.cpp)

target_include_directories(test_skiplist PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Try system-installed GTest first
find_package(GTest QUIET)
if(GTest_FOUND)
  message(STATUS "Using system GoogleTest")
  target_link_libraries(test_skiplist PRIVATE minilsm GTest::gtest_main)
else()
  # Fallback to third_party directory; user can run scripts/fetch_gtest.sh
  set(GTEST_ROOT ${CMAKE_SOURCE_DIR}/third_party/googletest/install CACHE PATH "Path to googletest install (run scripts/fetch_gtest.sh)")

  find_path(GTEST_INCLUDE_DIR NAMES gtest/gtest.h PATHS ${GTEST_ROOT}/include)
  find_library(GTEST_LIBRARY NAMES gtest PATHS ${GTEST_ROOT}/lib ${GTEST_ROOT}/lib64)
  find_library(GTEST_MAIN_LIBRARY NAMES gtest_main PATHS ${GTEST_ROOT}/lib ${GTEST_ROOT}/lib64)

  if(NOT GTEST_INCLUDE_DIR OR NOT GTEST_LIBRARY OR NOT GTEST_MAIN_LIBRARY)
    message(FATAL_ERROR "GoogleTest not found (system or ${GTEST_ROOT}). Run scripts/fetch_gtest.sh to download and build it, or set GTEST_ROOT to your gtest install.")
  endif()

  target_include_directories(test_skiplist PRIVATE ${GTEST_INCLUDE_DIR})
  target_link_libraries(test_skiplist PRIVATE minilsm ${GTEST_MAIN_LIBRARY} ${GTEST_LIBRARY} pthread)
endif()

target_compile_options(test_skiplist PRIVATE
        -O0                  # 强制禁用所有优化（核心）
        -g                   # 保留基础调试信息
        -ggdb                # 额外 GDB 调试信息（可选，推荐）
        -fno-omit-frame-pointer  # 保留栈帧，方便 GDB 回溯（可选）
        -Wall -Wextra -Werror    # 通用警告（可选）
    )

# Optional: AddressSanitizer for memory leak detection
option(ENABLE_ASAN "Enable AddressSanitizer for tests" OFF)
if(ENABLE_ASAN)
  message(STATUS "AddressSanitizer enabled for targets")
  target_compile_options(minilsm PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_libraries(minilsm PRIVATE -fsanitize=address)
  target_compile_options(test_skiplist PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_libraries(test_skiplist PRIVATE -fsanitize=address)
endif()

include(CTest)
add_test(NAME skiplist_test COMMAND test_skiplist)

# If valgrind is available, add a memcheck test that runs the test under valgrind
find_program(VALGRIND_EXECUTABLE NAMES valgrind)
if(VALGRIND_EXECUTABLE)
  add_test(NAME skiplist_memcheck COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --error-exitcode=1 $<TARGET_FILE:test_skiplist>)
  set_tests_properties(skiplist_memcheck PROPERTIES FAIL_REGULAR_EXPRESSION "definitely lost: 0 bytes")
endif()

# Install test executable for local runs
install(TARGETS test_skiplist
  RUNTIME DESTINATION bin
)
