cmake_minimum_required(VERSION 3.16)

project(Hazelnut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../bin/$<CONFIG>/${PROJECT_NAME}")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/../bin-int/$<CONFIG>/${PROJECT_NAME}")

# Source files
file(GLOB_RECURSE HAZELNUT_SOURCES
    "src/**.h"
    "src/**.cpp"
)

# Create console application
add_executable(Hazelnut
    ${HAZELNUT_SOURCES}
)

# Include directories
target_include_directories(Hazelnut PRIVATE
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor/spdlog/include
    ${PROJECT_SOURCE_DIR}/../Hazel/src
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor/entt/include
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor/filewatch
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor/glm
    ${PROJECT_SOURCE_DIR}/../Hazel/vendor/ImGuizmo
)

# Link libraries
target_link_libraries(Hazelnut PRIVATE
    Hazel
)

# Platform specific settings
if(WIN32)
    target_compile_definitions(Hazelnut PRIVATE
        _WIN32_WINNT=0x0A00
    )
endif()

# Configuration specific settings
target_compile_definitions(Hazelnut PRIVATE
    $<$<CONFIG:Debug>:HZ_DEBUG>
    $<$<CONFIG:Release>:HZ_RELEASE>
    $<$<CONFIG:Dist>:HZ_DIST>
)

# Runtime settings
set_target_properties(Hazelnut PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
)

# Set output directories explicitly per configuration
set_target_properties(Hazelnut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/../bin/Debug/${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/../bin/Release/${PROJECT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY_Dist "${PROJECT_SOURCE_DIR}/../bin/Dist/${PROJECT_NAME}"
    PDB_OUTPUT_DIRECTORY_DEBUG "${PROJECT_SOURCE_DIR}/../bin-int/Debug/${PROJECT_NAME}"
    PDB_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}/../bin-int/Release/${PROJECT_NAME}"
    PDB_OUTPUT_DIRECTORY_Dist "${PROJECT_SOURCE_DIR}/../bin-int/Dist/${PROJECT_NAME}"
    
    # 设置调试时的工作目录为 Hazelnut 项目目录（自动找到 assets 文件夹）
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)