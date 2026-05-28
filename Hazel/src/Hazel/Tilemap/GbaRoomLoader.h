#pragma once

#include "Hazel/Renderer/TileMapAsset.h"
#include "Hazel/Renderer/TileSetAsset.h"
#include "Hazel/Renderer/PaletteAsset.h"
#include "Hazel/Tilemap/RoomDef.h"

#include <vector>
#include <string>

namespace Hazel {

// ============================================================
// GbaRoomLoader — GBA 原生二进制 → 引擎 Asset
// ============================================================
//
// GBA 数据格式:
//   tileset  : 4bpp packed (2 pixels/byte)，每 subTile 8×8=64 像素=32 字节
//   palette  : BGR555 (2 bytes/color), 16 pal×16 color=256 色
//   tileindex: uint16_t 数组，每格一个 tileIndex
//   subtile  : uint16_t 数组，每个 tileIndex 对应 4 个 GBA BG map entry
//   tiletype : uint16_t 数组，tileIndex → TileType enum
//
// GBA BG map entry 格式 → 引擎 PackSubTile 格式转换:
//   GBA:   [15:12] pal | [11] flipV | [10] flipH | [9:0] subtileIdx
//   引擎:  [15:6] subtileIdx | [5] flipH | [4] flipV | [3:0] pal

class GbaRoomLoader
{
public:
	// ---------- 独立资源加载 (可缓存) ----------

	// 加载 GBA 4bpp tileset 图形 → TileSetAsset (上传 SSBO)
	// subtileCount: tileset 包含的 subTile 总数
	static Ref<TileSetAsset> LoadTileset(const std::string& filepath, uint32_t subtileCount);

	// 加载 GBA BGR555 调色板 → PaletteAsset (256 色 RGBA8, 上传 SSBO)
	// 若文件不存在则返回默认灰度 palette (16 pal × 16 灰阶)
	static Ref<PaletteAsset> LoadPalette(const std::string& filepath);

	// 生成默认灰度调色板 (16 palettes × 16 grayscale steps)
	static Ref<PaletteAsset> CreateDefaultPalette();

	// 加载 subtile 映射表，转换为引擎 PackSubTile 格式
	// tileCount: tileset 的 tile 数量, 返回长度 = tileCount * 4
	static std::vector<uint16_t> LoadSubTiles(const std::string& filepath, uint16_t tileCount);

	// 加载 tileType 表
	static std::vector<uint16_t> LoadTileTypes(const std::string& filepath, uint16_t tileCount);

	// 加载 tileindex (房间 map 数据)
	static std::vector<uint16_t> LoadMapData(const std::string& filepath, uint16_t width, uint16_t height);

	// ---------- 组合加载 ----------

	// 加载一个房间的完整 TileMapAsset (含 Top/Bottom 两层)
	static Ref<TileMapAsset> LoadRoom(const RoomDef& def);

private:
	// GBA subTile entry → 引擎 PackSubTile 格式
	static uint16_t ConvertSubTileEntry(uint16_t gbaEntry);
};

} // namespace Hazel
