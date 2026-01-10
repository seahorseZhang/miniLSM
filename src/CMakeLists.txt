cmake_minimum_required(VERSION 3.10)

add_library(minilsm SHARED memtable.cpp)
target_include_directories(minilsm PUBLIC ${CMAKE_SOURCE_DIR}/include)

set_target_properties(minilsm PROPERTIES OUTPUT_NAME "minilsm")

## Also provide a debug-build target that compiles the same sources
add_library(minilsm_debug SHARED memtable.cpp)
target_include_directories(minilsm_debug PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(minilsm_debug PROPERTIES OUTPUT_NAME "minilsm_debug")

# Set explicit build-type-ish flags for the debug target (portable via generator expressions)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(minilsm_debug PRIVATE -g -O0)
	target_compile_options(minilsm PRIVATE -O3)
elseif(MSVC)
	target_compile_options(minilsm_debug PRIVATE /Zi /Od)
	target_compile_options(minilsm PRIVATE /O2)
endif()

# Install rules for libraries and headers
install(TARGETS minilsm minilsm_debug
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)

# Install public headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# Tool: interactive visualizer
add_executable(skiplist_viz ${CMAKE_SOURCE_DIR}/tools/skiplist_viz.cpp)
target_include_directories(skiplist_viz PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(skiplist_viz PRIVATE minilsm)
install(TARGETS skiplist_viz RUNTIME DESTINATION bin)
