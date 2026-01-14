cmake_minimum_required(VERSION 3.10)

file(GLOB_RECURSE CORE_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

file(GLOB_RECURSE AUX_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.cpp
)

add_library(minilsm SHARED ${CORE_SRC_FILES} ${AUX_SRC_FILES})
target_include_directories(minilsm PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ========== 核心修改：按模式设置编译选项 ==========
# 生成器表达式 $<CONFIG:Debug> 匹配 Debug 模式，$<CONFIG:Release> 匹配 Release 模式
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC/Clang：Debug 模式 (-O0 + -g + -ggdb)，Release 模式 (-O3)
    target_compile_options(minilsm PRIVATE
        $<$<CONFIG:Debug>:-O0>          # Debug 禁用优化
        $<$<CONFIG:Debug>:-g>           # Debug 保留调试信息
        $<$<CONFIG:Debug>:-ggdb>        # Debug 额外 GDB 调试信息（可选，推荐）
        $<$<CONFIG:Release>:-O3>        # Release 开启最高优化
        -Wall -Wextra -Werror           # 通用警告（可选，推荐开启）
    )
elseif(MSVC)
    # MSVC：Debug 模式 (/Od 禁用优化 + /Zi 调试信息)，Release 模式 (/O2 优化)
    target_compile_options(minilsm PRIVATE
        $<$<CONFIG:Debug>:/Od>          # MSVC Debug 禁用优化
        $<$<CONFIG:Debug>:/Zi>          # MSVC Debug 生成调试信息
        $<$<CONFIG:Release>:/O2>        # MSVC Release 开启优化
        /W4 /WX                        # MSVC 警告级别（可选）
    )
endif()

# 设置输出名称（原有逻辑保留）
set_target_properties(minilsm PROPERTIES OUTPUT_NAME "minilsm")

# 安装规则（原有逻辑保留）
install(TARGETS minilsm
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include)

# 工具：跳表可视化（原有逻辑保留，同步适配编译模式）
add_executable(skiplist_viz ${CMAKE_SOURCE_DIR}/tools/skiplist_viz.cpp)
target_include_directories(skiplist_viz PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(skiplist_viz PRIVATE minilsm)
# 给可视化工具也应用相同的编译选项（可选，保持一致）
target_compile_options(skiplist_viz PRIVATE
    $<$<CONFIG:Debug>:-O0 -g -ggdb>
    $<$<CONFIG:Release>:-O3>
)
install(TARGETS skiplist_viz RUNTIME DESTINATION bin)