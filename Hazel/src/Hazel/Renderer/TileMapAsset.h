#pragma once

#include "TileSetAsset.h"
#include "PaletteAsset.h"

#include <cstdint>
#include <vector>

namespace Hazel {

	// Packed SubTile entry (uint16_t):
	//   [15:6] subtileIdx (10 bits, 0-1023)
	//   [5]    flipH
	//   [4]    flipV
	//   [3:0]  palIdx  (4 bits, 0-15)
	inline uint16_t PackSubTile(uint16_t subtileIdx, bool flipH, bool flipV, uint8_t palIdx)
	{
		return static_cast<uint16_t>(
			((subtileIdx & 0x3FF) << 6)
			| ((flipH ? 1u : 0u) << 5)
			| ((flipV ? 1u : 0u) << 4)
			| (palIdx & 0xF));
	}

	inline uint16_t UnpackSubtileIdx(uint16_t packed) { return (packed >> 6) & 0x3FF; }
	inline bool      UnpackFlipH(uint16_t packed)     { return ((packed >> 5) & 1) != 0; }
	inline bool      UnpackFlipV(uint16_t packed)     { return ((packed >> 4) & 1) != 0; }
	inline uint8_t   UnpackPalIdx(uint16_t packed)    { return packed & 0xF; }

	struct TileMapLayer
	{
		// 64×64 tile index array (width * height entries)
		std::vector<uint16_t> MapData;

		// tileIndex -> subtile base address within SubTiles array
		std::vector<uint16_t> TileIndices;

		// SubTile data: each uint16_t packs [subtileIdx:10 | flipH:1 | flipV:1 | palIdx:4]
		std::vector<uint16_t> SubTiles;

		// Per-tile type flags (for collision generation at runtime)
		std::vector<uint16_t> TileTypes;

		uint16_t TileCount = 0;
		uint16_t SubTileCount = 0;
	};

	class TileMapAsset
	{
	public:
		static Ref<TileMapAsset> Create()
		{
			return CreateRef<TileMapAsset>();
		}

		uint16_t GetWidth() const { return m_Width; }
		uint16_t GetHeight() const { return m_Height; }
		void SetDimensions(uint16_t w, uint16_t h) { m_Width = w; m_Height = h; }

		TileMapLayer Bottom;
		TileMapLayer Top;

		const std::string& GetSourcePath() const { return m_SourcePath; }
		void SetSourcePath(const std::string& path) { m_SourcePath = path; }

	private:
		std::string m_SourcePath;
		uint16_t m_Width = 0;
		uint16_t m_Height = 0;
	};

}
