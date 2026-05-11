
# vendor/glad/CMakeLists.txt（手动新增）
cmake_minimum_required(VERSION 3.8)
project(glad)

# 编译 GLAD 为静态库
add_library(glad STATIC
    src/glad.c
    include/glad/glad.h
    include/KHR/khrplatform.h
)

# 指定头文件路径
target_include_directories(glad
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 指定 C99 标准
set_target_properties(glad PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)