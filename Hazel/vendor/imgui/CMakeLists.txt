cmake_minimum_required(VERSION 3.10)
project(imgui LANGUAGES CXX)

set(IMGUI_CORE_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
)

set(IMGUI_BACKENDS_SOURCES
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp
)

set(IMGUI_HEADERS
    imgui.h
    imgui_internal.h
    imconfig.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
)

set(IMGUI_BACKENDS_HEADERS
    backends/imgui_impl_glfw.h
    backends/imgui_impl_opengl3.h
    backends/imgui_impl_opengl3_loader.h
)

add_library(imgui STATIC
    ${IMGUI_CORE_SOURCES}
    ${IMGUI_BACKENDS_SOURCES}
    ${IMGUI_HEADERS}
    ${IMGUI_BACKENDS_HEADERS}
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

if(TARGET glfw)
    target_link_libraries(imgui PUBLIC glfw)
    target_include_directories(imgui PUBLIC ${GLFW_INCLUDE_DIR})
else()
    target_include_directories(imgui PUBLIC
        ${CMAKE_SOURCE_DIR}/../vendor/glfw/include
    )
endif()

set_target_properties(imgui PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

target_compile_definitions(imgui PUBLIC)

if(MSVC)
    target_compile_options(imgui PRIVATE
        /W4 /WX
        /wd4244 /wd4305
    )
endif()
