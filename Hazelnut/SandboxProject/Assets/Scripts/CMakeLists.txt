cmake_minimum_required(VERSION 3.15)
project(Sandbox CSharp)

# 创建 C# 库目标
add_library(Sandbox SHARED
    Source/Camera.cs
    Source/Player.cs
)

# 设置 .NET Framework 版本
set_property(TARGET Sandbox PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

# 设置编译选项
set_property(TARGET Sandbox PROPERTY VS_DOTNET_REFERENCES
    "System"
    "System.Core"
)

# 链接到 Hazel-ScriptCore
target_link_libraries(Sandbox PRIVATE Hazel-ScriptCore)

# 设置目标平台
set_property(TARGET Sandbox PROPERTY VS_GLOBAL_PlatformTarget "x64")

# 配置特定设置
set_property(TARGET Sandbox PROPERTY VS_GLOBAL_DebugType "full")
set_property(TARGET Sandbox PROPERTY VS_GLOBAL_Optimize "false")

# VS C# 项目不遵循 CMAKE_LIBRARY_OUTPUT_DIRECTORY，用 POST_BUILD 复制到引擎加载路径
set(SANDBOX_OUTPUT "${PROJECT_SOURCE_DIR}/Binaries")
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SANDBOX_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Sandbox>"
        "${SANDBOX_OUTPUT}/$<TARGET_FILE_NAME:Sandbox>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_PDB_FILE:Sandbox>"
        "${SANDBOX_OUTPUT}/$<TARGET_PDB_FILE_NAME:Sandbox>"
    COMMENT "Copying Sandbox to Assets/Scripts/Binaries"
)

# 显示编译信息
message(STATUS "Sandbox output: ${SANDBOX_OUTPUT}/Sandbox.dll")
