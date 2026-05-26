#pragma once

#include "Hazel/Renderer/TileSetAsset.h"
#include "Hazel/Renderer/PaletteAsset.h"
#include "Hazel/Renderer/TileMapAsset.h"

#include <string>

namespace Hazel {

	class TileMapSerializer
	{
	public:
		static Ref<TileSetAsset> LoadTileset(const std::string& filepath);
		static Ref<PaletteAsset> LoadPalette(const std::string& filepath);
		static Ref<TileMapAsset> LoadTileMap(const std::string& filepath);

	private:
		static void LoadLayer(std::ifstream& file, TileMapLayer& layer, uint32_t layerSize);
	};

}
