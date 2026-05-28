cmake_minimum_required (VERSION 3.15)
# C# 只在 Visual Studio 生成器下可用
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    project("Hazel-App" LANGUAGES CXX CSharp)
else()
    project("Hazel-App" LANGUAGES CXX)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Mono 的预编译 .lib 使用 /MDd 动态 CRT，必须保持一致
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# ==============================================
# 1. 先构建 Hazel 库（子目录必须在使用前声明）
# ==============================================
add_subdirectory("Hazel")

# ==============================================
# 2. 构建应用程序可执行文件
# ==============================================
add_executable(Hazel-App
    "Sandbox/src/SandboxApp.cpp"
    "Sandbox/src/Sandbox2D.cpp"
    "Sandbox/src/Sandbox2D.h"
    "Sandbox/src/SandboxGbaRoom.cpp"
    "Sandbox/src/SandboxGbaRoom.h"
    "Sandbox/src/Test.h")

target_include_directories(Hazel-App PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Sandbox/src"
)

# 链接 Hazel 库（所有依赖已由 Hazel 传递）
target_link_libraries(Hazel-App PRIVATE Hazel)

# 输出目录优化（可选）
# 使用静态 CRT 与第三方库一致
set_target_properties(Hazel-App PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# ==============================================
# 3. 构建 Hazelnut 编辑器
# ==============================================
add_subdirectory("Hazelnut")

# ==============================================
# 4. 构建 C# 脚本项目（仅 VS 生成器支持）
# ==============================================
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    # 先构建 Hazel-ScriptCore
    add_subdirectory("Hazel-ScriptCore")

    # 再构建 Sandbox 脚本项目
    add_subdirectory("Hazelnut/SandboxProject/Assets/Scripts")
endif()