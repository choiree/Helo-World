#include "hzpch.h"
#include "TileMapSerializer.h"

#include "Hazel/Core/Buffer.h"
#include "Hazel/Renderer/StorageBuffer.h"

#include <fstream>
#include <cstring>

namespace Hazel {

	// .htileset binary format (v2):
	//   [4] magic "TSET"
	//   [2] version = 2
	//   [2] tileWidth = 8
	//   [4] pixelWidth  = 8  (1 byte per pixel per row)
	//   [4] pixelHeight    (= subtileCount * 8)
	//   [pixelWidth * pixelHeight] unpacked 1-byte-per-pixel data (value 0-15)
	//
	// Upload to SSBO: 4 pixels packed per uint32
	struct TilesetHeader
	{
		char magic[4];
		uint16_t version;
		uint16_t tileWidth;
		uint32_t pixelWidth;
		uint32_t pixelHeight;
	};

	// .hpal binary format:
	//   [4] magic "HPAL"
	//   [256 * 4] raw RGBA8 (16 palettes x 16 colors)
	// Uploaded to SSBO: 256 uint32s (R | G<<8 | B<<16 | A<<24)
	struct PaletteHeader
	{
		char magic[4];
	};

	// .hmap binary format:
	//   [4] magic "HMAP"
	//   [2] version
	//   [2] width
	//   [2] height
	//   Bottom layer:
	//     [2] tileCount
	//     [2] subTileCount
	//     [width*height*2] mapData
	//     [tileCount*2] tileIndices
	//     [subTileCount*2] subTiles (uint16_t: [subtileIdx:10 | flipH:1 | flipV:1 | palIdx:4])
	//     [tileCount] tileTypes
	//   Top layer:
	//     (same structure)
	struct HMapHeader
	{
		char magic[4];
		uint16_t version;
		uint16_t width;
		uint16_t height;
	};

	struct LayerHeader
	{
		uint16_t tileCount;
		uint16_t subTileCount;
	};

	static bool CheckMagic(const char* magic, const char* expected)
	{
		return std::memcmp(magic, expected, 4) == 0;
	}

	void TileMapSerializer::LoadLayer(std::ifstream& file, TileMapLayer& layer, uint32_t layerSize)
	{
		LayerHeader lh;
		file.read((char*)&lh, sizeof(lh));

		layer.TileCount = lh.tileCount;
		layer.SubTileCount = lh.subTileCount;

		// mapData
		layer.MapData.resize(layerSize);
		file.read((char*)layer.MapData.data(), layerSize * sizeof(uint16_t));

		// tileIndices
		layer.TileIndices.resize(lh.tileCount);
		file.read((char*)layer.TileIndices.data(), lh.tileCount * sizeof(uint16_t));

		// subTiles (uint16_t packed)
		layer.SubTiles.resize(lh.subTileCount);
		file.read((char*)layer.SubTiles.data(), lh.subTileCount * sizeof(uint16_t));

		// tileTypes
		layer.TileTypes.resize(lh.tileCount);
		file.read((char*)layer.TileTypes.data(), lh.tileCount * sizeof(uint16_t));
	}

	Ref<TileSetAsset> TileMapSerializer::LoadTileset(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("Failed to open tileset file: {0}", filepath);
			return nullptr;
		}

		TilesetHeader header;
		file.read((char*)&header, sizeof(header));

		if (!CheckMagic(header.magic, "TSET"))
		{
			HZ_CORE_ERROR("Invalid tileset magic in: {0}", filepath);
			return nullptr;
		}

		if (header.version < 2)
		{
			HZ_CORE_ERROR("Unsupported tileset version {0} in: {1}", header.version, filepath);
			return nullptr;
		}

		// v2: pre-unpacked, 1 byte per pixel (value 0-15), 8 bytes per row
		const uint32_t pixelW = header.pixelWidth;  // 8
		const uint32_t subtileCount = header.pixelHeight / 8;
		const uint32_t pixelsPerSubtile = 64;  // 8×8
		uint32_t totalPixels = subtileCount * pixelsPerSubtile;
		uint32_t ssboWords = (totalPixels + 3) / 4;

		uint32_t rawSize = pixelW * header.pixelHeight;
		Buffer rawData(rawSize);
		file.read((char*)rawData.Data, rawSize);

		if (file.fail())
		{
			HZ_CORE_ERROR("Failed to read tileset pixel data from: {0}", filepath);
			return nullptr;
		}

		Buffer ssboData(ssboWords * sizeof(uint32_t));
		std::memset(ssboData.Data, 0, ssboWords * sizeof(uint32_t));
		uint32_t* dst = reinterpret_cast<uint32_t*>(ssboData.Data);
		const uint8_t* src = reinterpret_cast<const uint8_t*>(rawData.Data);

		for (uint32_t s = 0; s < subtileCount; s++)
		{
			for (uint32_t y = 0; y < 8; y++)
			{
				uint32_t srcOff = s * pixelsPerSubtile + y * pixelW;
				for (uint32_t x = 0; x < 8; x++)
				{
					uint32_t pixelBase = s * pixelsPerSubtile + y * 8 + x;
					uint32_t wordIdx = pixelBase / 4;
					uint32_t shift = (pixelBase & 3) * 8;
					dst[wordIdx] |= (static_cast<uint32_t>(src[srcOff + x]) << shift);
				}
			}
		}

		auto ssbo = StorageBuffer::Create(ssboWords * sizeof(uint32_t), 1);
		ssbo->SetData(ssboData.Data, ssboWords * sizeof(uint32_t));

		auto asset = TileSetAsset::Create();
		asset->SetBuffer(ssbo);
		asset->SetTileWidth(header.tileWidth);
		asset->SetSubTileCount(subtileCount);
		asset->SetSourcePath(filepath);
		return asset;
	}

	Ref<PaletteAsset> TileMapSerializer::LoadPalette(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("Failed to open palette file: {0}", filepath);
			return nullptr;
		}

		PaletteHeader header;
		file.read((char*)&header, sizeof(header));

		if (!CheckMagic(header.magic, "HPAL"))
		{
			HZ_CORE_ERROR("Invalid palette magic in: {0}", filepath);
			return nullptr;
		}

		const uint32_t colorCount = 256;
		const uint32_t rawSize = colorCount * 4;
		Buffer rawData(rawSize);
		file.read((char*)rawData.Data, rawSize);

		if (file.fail())
		{
			HZ_CORE_ERROR("Failed to read palette data from: {0}", filepath);
			return nullptr;
		}

		// Pack RGBA8 into uint32: R | (G<<8) | (B<<16) | (A<<24)
		Buffer ssboData(colorCount * sizeof(uint32_t));
		const uint8_t* src = reinterpret_cast<const uint8_t*>(rawData.Data);
		uint32_t* dst = reinterpret_cast<uint32_t*>(ssboData.Data);

		for (uint32_t i = 0; i < colorCount; i++)
		{
			uint32_t off = i * 4;
			dst[i] = static_cast<uint32_t>(src[off + 0])
				| (static_cast<uint32_t>(src[off + 1]) << 8)
				| (static_cast<uint32_t>(src[off + 2]) << 16)
				| (static_cast<uint32_t>(src[off + 3]) << 24);
		}

		auto ssbo = StorageBuffer::Create(colorCount * sizeof(uint32_t), 2);
		ssbo->SetData(ssboData.Data, colorCount * sizeof(uint32_t));

		auto asset = PaletteAsset::Create();
		asset->SetBuffer(ssbo);
		asset->SetSourcePath(filepath);
		return asset;
	}

	Ref<TileMapAsset> TileMapSerializer::LoadTileMap(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::binary);
		if (!file.is_open())
		{
			HZ_CORE_ERROR("Failed to open tilemap file: {0}", filepath);
			return nullptr;
		}

		HMapHeader header;
		file.read((char*)&header, sizeof(header));

		if (!CheckMagic(header.magic, "HMAP"))
		{
			HZ_CORE_ERROR("Invalid tilemap magic in: {0}", filepath);
			return nullptr;
		}

		auto asset = TileMapAsset::Create();
		asset->SetDimensions(header.width, header.height);
		asset->SetSourcePath(filepath);

		uint32_t layerSize = header.width * header.height;

		LoadLayer(file, asset->Bottom, layerSize);
		LoadLayer(file, asset->Top, layerSize);

		if (file.fail())
		{
			HZ_CORE_ERROR("Failed to read tilemap data from: {0}", filepath);
			return nullptr;
		}

		return asset;
	}

}
