#include "hzpch.h"
#include "GbaRoomLoader.h"

#include "Hazel/Core/Buffer.h"
#include "Hazel/Renderer/StorageBuffer.h"

#include <fstream>
#include <cstring>

namespace Hazel {

	// ============================================================
	// GBA → 引擎 subTile 格式转换
	// ============================================================

	uint16_t GbaRoomLoader::ConvertSubTileEntry(uint16_t gba)
	{
		// GBA:   [15:12] pal | [11] flipV | [10] flipH | [9:0] subtileIdx
		// 引擎:  [15:6] subtileIdx | [5] flipH | [4] flipV | [3:0] pal
		uint16_t idx   = (gba & 0x3FF);
		bool     flipH = (gba >> 10) & 1;
		bool     flipV = (gba >> 11) & 1;
		uint8_t  pal   = (gba >> 12) & 0xF;
		return PackSubTile(idx, flipH, flipV, pal);
	}

	// ============================================================
	// Tileset 加载: GBA 4bpp packed → SSBO (4 pixels/uint32)
	// ============================================================

	Ref<TileSetAsset> GbaRoomLoader::LoadTileset(const std::string& filepath, uint32_t subtileCount)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to open tileset: {0}", filepath);
			return nullptr;
		}

		// 4bpp: 每 subTile 8×8=64 pixels = 32 bytes
		const uint32_t pixelsPerSubtile = 64;
		const uint32_t bytesPerSubtile  = 32;
		uint32_t totalBytes = subtileCount * bytesPerSubtile;

		Buffer rawData(totalBytes);
		file.read((char*)rawData.Data, totalBytes);
		if (file.fail())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to read tileset data: {0}", filepath);
			return nullptr;
		}

		// 解包 4bpp → 1 byte/pixel → 打包 4 pixels/uint32 (SSBO 格式)
		uint32_t totalPixels = subtileCount * pixelsPerSubtile;
		uint32_t ssboWords   = (totalPixels + 3) / 4;

		Buffer ssboData(ssboWords * sizeof(uint32_t));
		std::memset(ssboData.Data, 0, ssboData.Size);
		uint32_t* dst = (uint32_t*)ssboData.Data;
		const uint8_t* src = rawData.Data;

		for (uint32_t s = 0; s < subtileCount; s++)
		{
			for (uint32_t y = 0; y < 8; y++)
			{
				for (uint32_t x = 0; x < 8; x++)
				{
					uint32_t byteOff = s * bytesPerSubtile + y * 4 + x / 2;
					uint8_t packed = src[byteOff];
					// 低半字节优先 (GBA 4bpp 标准: low nibble = left pixel)
					uint8_t pixel = (x & 1) ? (packed >> 4) : (packed & 0xF);

					uint32_t pixelBase = s * pixelsPerSubtile + y * 8 + x;
					uint32_t wordIdx   = pixelBase / 4;
					uint32_t shift     = (pixelBase & 3) * 8;
					dst[wordIdx] |= (static_cast<uint32_t>(pixel) << shift);
				}
			}
		}

		auto ssbo = StorageBuffer::Create(ssboWords * sizeof(uint32_t), 1);
		ssbo->SetData(ssboData.Data, ssboWords * sizeof(uint32_t));

		auto asset = TileSetAsset::Create();
		asset->SetBuffer(ssbo);
		asset->SetTileWidth(8);
		asset->SetSubTileCount(subtileCount);
		asset->SetSourcePath(filepath);
		return asset;
	}

	// ============================================================
	// Palette 加载: GBA BGR555 → RGBA8 SSBO
	// ============================================================

	Ref<PaletteAsset> GbaRoomLoader::CreateDefaultPalette()
	{
		// 16 palettes × 16 grayscale steps = 256 colors RGBA8
		const uint32_t colorCount = 256;
		Buffer ssboData(colorCount * sizeof(uint32_t));
		uint32_t* dst = (uint32_t*)ssboData.Data;

		for (uint32_t p = 0; p < 16; p++)
		{
			for (uint32_t c = 0; c < 16; c++)
			{
				uint8_t v = (uint8_t)(c * 17); // 0, 17, 34, ..., 255
				uint32_t i = p * 16 + c;
				dst[i] = (uint32_t)v | ((uint32_t)v << 8) | ((uint32_t)v << 16) | (0xFFu << 24);
			}
		}

		auto ssbo = StorageBuffer::Create(colorCount * sizeof(uint32_t), 2);
		ssbo->SetData(ssboData.Data, colorCount * sizeof(uint32_t));

		auto asset = PaletteAsset::Create();
		asset->SetBuffer(ssbo);
		asset->SetSourcePath("(default grayscale)");
		return asset;
	}

	Ref<PaletteAsset> GbaRoomLoader::LoadPalette(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_WARN("GbaRoomLoader: Palette not found, using default grayscale: {0}", filepath);
			return CreateDefaultPalette();
		}

		// 16 palettes × 16 colors = 256 colors, 2 bytes each
		const uint32_t colorCount = 256;
		const uint32_t rawSize = colorCount * 2;

		Buffer rawData(rawSize);
		file.read((char*)rawData.Data, rawSize);
		if (file.fail())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to read palette data: {0}", filepath);
			return nullptr;
		}

		// BGR555 → RGBA8
		Buffer ssboData(colorCount * sizeof(uint32_t));
		const uint8_t* src = rawData.Data;
		uint32_t* dst = (uint32_t*)ssboData.Data;

		for (uint32_t i = 0; i < colorCount; i++)
		{
			uint16_t bgr = (uint16_t)src[i * 2] | ((uint16_t)src[i * 2 + 1] << 8);

			// 展开 5-bit → 8-bit (左移 3 位 + 高位复制: (x<<3) | (x>>2))
			uint8_t r = ((bgr & 0x1F)       * 255 + 15) / 31;
			uint8_t g = (((bgr >> 5) & 0x1F) * 255 + 15) / 31;
			uint8_t b = (((bgr >> 10) & 0x1F) * 255 + 15) / 31;

			dst[i] = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | (0xFF << 24);
		}

		auto ssbo = StorageBuffer::Create(colorCount * sizeof(uint32_t), 2);
		ssbo->SetData(ssboData.Data, colorCount * sizeof(uint32_t));

		auto asset = PaletteAsset::Create();
		asset->SetBuffer(ssbo);
		asset->SetSourcePath(filepath);
		return asset;
	}

	// ============================================================
	// SubTile 映射加载
	// ============================================================

	std::vector<uint16_t> GbaRoomLoader::LoadSubTiles(const std::string& filepath, uint16_t tileCount)
	{
		std::vector<uint16_t> result(tileCount * 4);

		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to open subtiles: {0}", filepath);
			return result;
		}

		for (uint32_t i = 0; i < (uint32_t)tileCount * 4; i++)
		{
			uint16_t gbaEntry = 0;
			file.read((char*)&gbaEntry, 2);
			result[i] = ConvertSubTileEntry(gbaEntry);
		}

		return result;
	}

	// ============================================================
	// TileType 加载
	// ============================================================

	std::vector<uint16_t> GbaRoomLoader::LoadTileTypes(const std::string& filepath, uint16_t tileCount)
	{
		std::vector<uint16_t> result(tileCount);

		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to open tiletypes: {0}", filepath);
			return result;
		}

		file.read((char*)result.data(), tileCount * sizeof(uint16_t));
		return result;
	}

	// ============================================================
	// MapData (tileindex) 加载
	// ============================================================

	std::vector<uint16_t> GbaRoomLoader::LoadMapData(const std::string& filepath, uint16_t width, uint16_t height)
	{
		uint32_t count = (uint32_t)width * height;
		std::vector<uint16_t> result(count);

		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("GbaRoomLoader: Failed to open map data: {0}", filepath);
			return result;
		}

		file.read((char*)result.data(), count * sizeof(uint16_t));
		return result;
	}

	// ============================================================
	// Room 组合加载
	// ============================================================

	static void LoadLayer(
		const std::string& mapPath,
		const std::string& subTilePath,
		const std::string& tileTypePath,
		uint16_t tilesWide, uint16_t tilesHigh,
		uint16_t tileCount,
		TileMapLayer& layer)
	{
		layer.MapData   = GbaRoomLoader::LoadMapData(mapPath, tilesWide, tilesHigh);
		layer.SubTiles  = GbaRoomLoader::LoadSubTiles(subTilePath, tileCount);
		layer.TileTypes = GbaRoomLoader::LoadTileTypes(tileTypePath, tileCount);
		layer.TileCount = tileCount;
		layer.SubTileCount = (uint16_t)layer.SubTiles.size();

		// 构建反向索引: tileType → tileIndex
		layer.TileIndices.resize(tileCount);
		for (uint16_t i = 0; i < tileCount; i++)
		{
			uint16_t type = layer.TileTypes[i];
			if (type < tileCount)
				layer.TileIndices[type] = i;
		}
	}

	Ref<TileMapAsset> GbaRoomLoader::LoadRoom(const RoomDef& def)
	{
		auto asset = TileMapAsset::Create();
		uint16_t tw = def.Header.TilesWide();
		uint16_t th = def.Header.TilesHigh();
		asset->SetDimensions(tw, th);
		asset->SetSourcePath(def.BottomMapPath);

		// Bottom layer — 从 bottom subtile 推断 tileCount
		uint16_t bottomTileCount = 256;
		{
			std::ifstream f(def.BottomSubTilePath, std::ios::binary | std::ios::ate);
			if (f.is_open())
			{
				uint32_t size = (uint32_t)f.tellg();
				bottomTileCount = (uint16_t)(size / 8);
				if (bottomTileCount == 0) bottomTileCount = 256;
			}
		}

		LoadLayer(def.BottomMapPath, def.BottomSubTilePath, def.BottomTileTypePath,
			tw, th, bottomTileCount, asset->Bottom);

		if (!def.TopMapPath.empty())
		{
			uint16_t topTileCount = 256;
			{
				std::ifstream f(def.TopSubTilePath, std::ios::binary | std::ios::ate);
				if (f.is_open())
				{
					uint32_t size = (uint32_t)f.tellg();
					topTileCount = (uint16_t)(size / 8);
					if (topTileCount == 0) topTileCount = 256;
				}
			}

			LoadLayer(def.TopMapPath, def.TopSubTilePath, def.TopTileTypePath,
				tw, th, topTileCount, asset->Top);
		}

		return asset;
	}

} // namespace Hazel
