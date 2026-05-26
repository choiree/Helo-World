// TileMap Shader — SSBO tileset + SSBO palette → RGBA
//   Tileset SSBO (binding=1): packed uint32[ ], 4 pixels per word (1 byte each, value 0-15)
//   Palette SSBO (binding=2): 256 uint32s, each RGBA8 packed: R | G<<8 | B<<16 | A<<24
//   SubTile uint16: [15:6]subtileIdx(10) | [5]flipH | [4]flipV | [3:0]palIdx(4)

#type vertex
#version 450 core

layout(location = 0) in vec2 a_TexCoord;
layout(location = 1) in int a_GridX;
layout(location = 2) in int a_GridY;
layout(location = 3) in int a_PackedData;
layout(location = 4) in int a_EntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjection;
};

uniform mat4 u_Model;

out vec2 v_TexCoord;
flat out uint v_PackedData;
flat out int v_EntityID;

void main()
{
	v_TexCoord = a_TexCoord;
	v_PackedData = uint(a_PackedData);
	v_EntityID = a_EntityID;

	vec2 worldPos = vec2(float(a_GridX), float(a_GridY)) + a_TexCoord * 8.0;
	gl_Position = u_ViewProjection * u_Model * vec4(worldPos, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

in vec2 v_TexCoord;
flat in uint v_PackedData;
flat in int v_EntityID;

layout(std430, binding = 1) readonly buffer TilesetData
{
	uint tilesetPixels[];   // 4 pixels per uint, 1 byte each (value 0-15)
};

layout(std430, binding = 2) readonly buffer PaletteData
{
	uint paletteColors[];   // 256 entries, each RGBA8 packed into uint32
};

void main()
{
	// Unpack subTile data (uint16_t)
	uint subtileIdx = (v_PackedData >> 6) & 0x3FFu;
	bool flipH = ((v_PackedData >> 5) & 1u) != 0u;
	bool flipV = ((v_PackedData >> 4) & 1u) != 0u;
	uint palIdx  = v_PackedData & 0xFu;

	// Apply flip
	vec2 tc = v_TexCoord;
	if (flipH) tc.x = 1.0 - tc.x;
	if (flipV) tc.y = 1.0 - tc.y;

	// Pixel position within 8×8 subtile
	uint px = uint(tc.x * 8.0);
	uint py = uint(tc.y * 8.0);
	uint pixelIdx = subtileIdx * 64u + py * 8u + px;

	// Read pixel value from tileset SSBO: 4 pixels per uint32
	uint word = tilesetPixels[pixelIdx / 4u];
	uint shift = (pixelIdx & 3u) * 8u;
	uint colorIdx = (word >> shift) & 0xFFu;

	// Palette lookup: palIdx × 16 + colorIdx
	uint rgba = paletteColors[palIdx * 16u + colorIdx];

	// Unpack RGBA
	float r = float(rgba & 0xFFu) / 255.0;
	float g = float((rgba >> 8) & 0xFFu) / 255.0;
	float b = float((rgba >> 16) & 0xFFu) / 255.0;
	float a = float((rgba >> 24) & 0xFFu) / 255.0;

	if (a == 0.0)
		discard;

	o_Color = vec4(r, g, b, a);
	o_EntityID = v_EntityID;
}
