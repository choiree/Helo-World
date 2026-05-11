# 3rd/stb/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(stb VERSION 2.82 LANGUAGES C CXX) # 版本可根据实际stb版本更新

# 1. 核心：定义stb为INTERFACE库（头文件库专用，无编译输出，仅传递配置）
add_library(stb INTERFACE)
# 定义别名stb::stb，方便项目中统一引用（避免与其他库重名）
add_library(stb::stb ALIAS stb)

# 2. 关键：设置stb的头文件目录（让项目能直接#include <stb_xxx.h>）
# 无需项目源码中写相对路径（如#include "../../3rd/stb/stb_image.h"），简化引用
target_include_directories(stb
    INTERFACE
        # 构建阶段：指向stb当前目录（库目录内的头文件路径）
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # 可选：安装阶段路径（若需将stb安装到系统/项目输出目录，保留此行）
        $<INSTALL_INTERFACE:include/stb>
)

# 3. 按需定义stb模块的实现宏（核心：避免重复定义实现，统一管理）
# 每个stb模块需通过STB_XXX_IMPLEMENTATION生成实现，此处统一配置
# 可根据项目需求，添加/注释对应模块（无需修改源码，灵活启用）
target_compile_definitions(stb
    INTERFACE
        STB_IMAGE_IMPLEMENTATION          # 图片加载模块（常用）
        STB_IMAGE_WRITE_IMPLEMENTATION    # 图片写入模块（常用）
        STB_TRUETYPE_IMPLEMENTATION       # 字体渲染模块（按需启用）
        STB_RECT_PACK_IMPLEMENTATION      # 矩形打包模块（按需启用）
        # STB_VORBIS_IMPLEMENTATION       # 音频解码模块（如需启用，取消注释）
        # STB_EASY_FONT_IMPLEMENTATION    # 简易字体模块（如需启用，取消注释）
)

