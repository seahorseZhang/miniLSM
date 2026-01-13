cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE CORE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

file(GLOB_RECURSE AUX_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

add_library(minilsm SHARED ${CORE_SRC_FILES} ${AUX_SRC_FILES})
target_include_directories(minilsm PUBLIC ${CMAKE_SOURCE_DIR}/include)

set_target_properties(minilsm PROPERTIES OUTPUT_NAME "minilsm")

# Set explicit build-type-ish flags for the debug target (portable via generator expressions)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(minilsm PRIVATE -O3)
elseif(MSVC)
	target_compile_options(minilsm PRIVATE /O2)
endif()

# Install rules for libraries and headers
install(TARGETS minilsm
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
