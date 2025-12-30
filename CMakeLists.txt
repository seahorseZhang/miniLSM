cmake_minimum_required(VERSION 3.10)
project(minilsm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# Build library and tests in subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Convenience: build and run the visualizer with example commands
find_program(DOT_EXECUTABLE dot)
add_custom_target(run-viz
	DEPENDS skiplist_viz
	COMMAND ${CMAKE_COMMAND} -E echo "Running skiplist_viz with example commands..."
	COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_BINARY_DIR}/src ./skiplist_viz ${CMAKE_SOURCE_DIR}/tools/example_commands.txt
	COMMENT "Builds skiplist_viz and runs it to emit a DOT file (build/src/example.dot)"
)
if(DOT_EXECUTABLE)
	add_custom_command(TARGET run-viz POST_BUILD
		COMMAND ${DOT_EXECUTABLE} -Tpng ${CMAKE_BINARY_DIR}/src/example.dot -o ${CMAKE_BINARY_DIR}/src/example.png
		COMMENT "Converting DOT to PNG using dot"
	)
endif()
