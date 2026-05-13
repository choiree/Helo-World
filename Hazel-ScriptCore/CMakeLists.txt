cmake_minimum_required(VERSION 3.15)
project(Hazel-ScriptCore CSharp)

# 设置输出目录（直接输出到 Resources/Scripts，不创建 Debug 子目录）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")

# 确保所有配置都输出到同一目录
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} CONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${PROJECT_SOURCE_DIR}/../Hazelnut/Resources/Scripts")
endforeach()

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

# 设置编译选项
set_property(TARGET Hazel-ScriptCore PROPERTY VS_DOTNET_REFERENCES
    "System"
    "System.Core"
)

# 设置目标平台
set_property(TARGET Hazel-ScriptCore PROPERTY VS_GLOBAL_PlatformTarget "x64")

# 显示编译信息
message(STATUS "Hazel-ScriptCore output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Hazel-ScriptCore.dll")
