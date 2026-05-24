cmake_minimum_required(VERSION 3.15)
project(Hazel LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ==============================
# 检查是否有 Mono
# ==============================
set(HZ_MONO_AVAILABLE FALSE)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono/include")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono/lib")
        set(HZ_MONO_AVAILABLE TRUE)
    endif()
endif()

# ==============================
# MSVC 运行时库设置（必须在添加子目录之前设置）
# ==============================
# Mono 的预编译 .lib 使用 /MDd 动态 CRT，所有项目保持一致
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# ==============================
# 源文件收集
# ==============================
file(GLOB_RECURSE HAZEL_SOURCES
    "src/*.h"
    "src/*.cpp"
    "vendor/stb_image/*.h"
    "vendor/stb_image/*.cpp"
    "vendor/glm/glm/*.hpp"
    "vendor/glm/glm/*.inl"
)

# 临时排除 Font.cpp — 需要 freetype 但暂时没有正确配置
list(FILTER HAZEL_SOURCES EXCLUDE REGEX ".*Renderer/Font\\.cpp$")

# 排除 ScriptEngine 源文件（如果没有 Mono）
if(NOT HZ_MONO_AVAILABLE)
    list(FILTER HAZEL_SOURCES EXCLUDE REGEX ".*Scripting/.*$")
endif()

file(GLOB IMGUIZMO_SOURCES
    "vendor/ImGuizmo/ImGuizmo.h"
    "vendor/ImGuizmo/ImGuizmo.cpp"
)

# ==============================
# 构建静态库
# ==============================
add_library(Hazel STATIC
    ${HAZEL_SOURCES}
    ${IMGUIZMO_SOURCES}
    "src/Hazel/Core/JobSystem/ThreadPool.cpp")

# ==============================
# 先添加所有 vendor 子目录
# ==============================
add_subdirectory("vendor/spdlog")
add_subdirectory("vendor/Box2D")
add_subdirectory("vendor/FileWatch")
add_subdirectory("vendor/glad")
add_subdirectory("vendor/glfw")
add_subdirectory("vendor/glm")
add_subdirectory("vendor/imgui")
add_subdirectory("vendor/imguizmo")
# add_subdirectory("vendor/msdf-atlas-gen")  # 暂时跳过 — Font.cpp 已排除
add_subdirectory("vendor/stb")
add_subdirectory("vendor/yaml-cpp")

# shaderc / spirv_cross
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)
add_subdirectory("vendor/shaderc")
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
add_subdirectory("vendor/spirv_cross")

# ==============================
# 头文件包含目录
# ==============================
target_include_directories(Hazel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yaml-cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/entt/single_include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/box2d/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono/mono
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/shaderc/libshaderc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spirv_cross/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/msdf-atlas-gen
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_include_directories(imguizmo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)

set(HAZEL_PCH_HEADER "src/Hzpch.h")
target_precompile_headers(Hazel PRIVATE ${HAZEL_PCH_HEADER})

target_compile_definitions(Hazel
    PRIVATE
    HZ_PLATFORM_WINDOWS
    HZ_STATIC
    HZ_RENDER_STATS
    GLFW_INCLUDE_NONE
    NOMINMAX
)

if(HZ_MONO_AVAILABLE)
    target_compile_definitions(Hazel PRIVATE HZ_ENABLE_SCRIPTING)
endif()

# ==============================
# 链接所有依赖库
# ==============================
target_link_libraries(Hazel PUBLIC
    glfw
    glad
    imgui
    imguizmo
    yaml-cpp
    spdlog
    box2d
    # msdf-atlas-gen  # 暂时跳过
    stb
    shaderc
    spirv-cross-core
    spirv-cross-glsl
)

if(HZ_MONO_AVAILABLE)
    target_link_libraries(Hazel PUBLIC
        # Mono
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/mono/lib/$<CONFIG>/libmono-static-sgen.lib
        # Mono 静态库需要的系统库
        legacy_stdio_definitions
        shlwapi
        psapi
        dbghelp
        winmm
        version
        ws2_32
        advapi32
        user32
        ole32
        oleaut32
        shell32
        uuid
    )
endif()

# Windows 系统库
target_link_libraries(Hazel PUBLIC
    ws2_32
    version
    winmm
    bcrypt
)

# 输出目录
set_target_properties(Hazel PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)
