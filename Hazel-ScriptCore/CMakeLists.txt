cmake_minimum_required(VERSION 3.15)
project(Hazel-ScriptCore CSharp)

# 创建 C# 库目标
add_library(Hazel-ScriptCore SHARED
    Source/Hazel/Input.cs
    Source/Hazel/InternalCalls.cs
    Source/Hazel/KeyCode.cs
    Source/Hazel/Vector2.cs
    Source/Hazel/Vector3.cs
    Source/Hazel/Vector4.cs
    Source/Hazel/Scene/Components.cs
    Source/Hazel/Scene/Entity.cs
)

# 设置 .NET Framework 版本
set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

# 设置编译选项
set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_REFERENCES
    "System"
    "System.Core"
)

# 设置目标平台
set_property(TARGET Hazel-ScriptCore PROPERTY VS_GLOBAL_PlatformTarget "x64")

# VS C# 项目不遵循 CMAKE_LIBRARY_OUTPUT_DIRECTORY，用 POST_BUILD 复制到引擎加载路径
set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
add_custom_command(TARGET Hazel-ScriptCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Hazel-ScriptCore>"
        "${SCRIPTS_DIR}/$<TARGET_FILE_NAME:Hazel-ScriptCore>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_PDB_FILE:Hazel-ScriptCore>"
        "${SCRIPTS_DIR}/$<TARGET_PDB_FILE_NAME:Hazel-ScriptCore>"
    COMMENT "Copying Hazel-ScriptCore to Resources/Scripts"
)

# 显示编译信息
message(STATUS "Hazel-ScriptCore output: ${SCRIPTS_DIR}/Hazel-ScriptCore.dll")
