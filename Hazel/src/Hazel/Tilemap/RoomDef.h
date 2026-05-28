#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Hazel {

// ============================================================
// RoomHeader — 与 GBA room_headers.s 逐字段对应
// ============================================================

struct RoomHeader
{
	uint16_t MapX        = 0;   // 在 Area 大地图上的 X 格位置 (tile 坐标)
	uint16_t MapY        = 0;   // 在 Area 大地图上的 Y 格位置 (tile 坐标)
	uint16_t PixelWidth  = 0;   // 房间像素宽 (16 的倍数)
	uint16_t PixelHeight = 0;   // 房间像素高 (16 的倍数)
	uint16_t TileSetId   = 0;   // 引用的 tileset 编号

	uint16_t TilesWide() const { return PixelWidth  / 16; }
	uint16_t TilesHigh() const { return PixelHeight / 16; }
};

// ============================================================
// RoomDef — 一个房间的完整定义 (元数据 + 数据文件路径)
// ============================================================

struct RoomDef
{
	RoomHeader Header;

	std::string BottomMapPath;       // 底层 tileindex 二进制
	std::string TopMapPath;          // 顶层 tileindex 二进制
	std::string BottomSubTilePath;   // 底层 subtile 映射表
	std::string TopSubTilePath;      // 顶层 subtile 映射表
	std::string BottomTileTypePath;  // 底层碰撞类型表
	std::string TopTileTypePath;     // 顶层碰撞类型表
	std::string TileSetPath;         // 图形 tileset (4bpp 像素)
	std::string PalettePath;         // 调色板 (BGR555)

	uint8_t Index = 0;           // 同一 area 内第几个 room
};

// ============================================================
// AreaDef — Area = Room[] + 元信息
// ============================================================

enum class AreaId : uint8_t
{
	// 仅列出有 room_headers 数据的 area，完整列表见 tmc-master include/area.h
	MinishWoods = 0,
	MinishVillage,
	HyruleTown,
	HyruleField,
	CastorWilds,
	Ruins,
	MtCrenel,
	CastleGarden,
	CloudTops,
	RoyalValley,
	VeilFalls,
	LakeHylia,
	// ...
	COUNT
};

struct AreaDef
{
	AreaId              Id;
	std::string         Name;
	std::vector<RoomDef> Rooms;   // 按 room_headers.s 顺序，索引即 roomIdx
};

// ============================================================
// RuntimeRoomInfo — 当前房间运行时状态
// ============================================================

struct RuntimeRoomInfo
{
	AreaId   CurrentArea  = AreaId::MinishWoods;
	uint8_t  CurrentRoom  = 0;            // AreaDef::Rooms 中的索引
	uint16_t PlayerTileX  = 0;            // 玩家当前 tile 坐标
	uint16_t PlayerTileY  = 0;
};

} // namespace Hazel
