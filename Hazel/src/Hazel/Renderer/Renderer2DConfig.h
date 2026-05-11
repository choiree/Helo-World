// Renderer2DConfig.h
#pragma once

namespace Hazel {

    // 着色器路径
    inline constexpr const char* QUAD_SHADER_PATH = "assets/shaders/Renderer2D_Quad.glsl";
    inline constexpr const char* CIRCLE_SHADER_PATH = "assets/shaders/Renderer2D_Circle.glsl";
    inline constexpr const char* LINE_SHADER_PATH = "assets/shaders/Renderer2D_Line.glsl";
    inline constexpr const char* TEXT_SHADER_PATH = "assets/shaders/Renderer2D_Text.glsl";

    // 顶点属性名称
    inline constexpr const char* ATT_POSITION = "a_Position";
    inline constexpr const char* ATT_COLOR = "a_Color";
    inline constexpr const char* ATT_TEX_COORD = "a_TexCoord";
    inline constexpr const char* ATT_TEX_INDEX = "a_TexIndex";
    inline constexpr const char* ATT_TILING = "a_TilingFactor";
    inline constexpr const char* ATT_ENTITY_ID = "a_EntityID";

    // UBO 绑定点
    inline constexpr uint32_t CAMERA_UBO_BINDING = 0;

    // 单位矩形顶点
    inline constexpr glm::vec4 QUAD_VERTS[4] = {
        { -0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f, -0.5f, 0.0f, 1.0f },
        {  0.5f,  0.5f, 0.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f, 1.0f }
    };

}
