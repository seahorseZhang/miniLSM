cmake_minimum_required(VERSION 3.10)
project(minilsm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# Build library and tests in subdirectories
add_subdirectory(src)
add_subdirectory(tests)
