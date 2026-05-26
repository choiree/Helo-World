#pragma once

#include "Hazel/Renderer/StorageBuffer.h"

namespace Hazel {

	class TileSetAsset
	{
	public:
		static Ref<TileSetAsset> Create()
		{
			return CreateRef<TileSetAsset>();
		}

		Ref<StorageBuffer> GetBuffer() const { return m_Buffer; }
		void SetBuffer(const Ref<StorageBuffer>& buf) { m_Buffer = buf; }

		uint32_t GetTileWidth() const { return m_TileWidth; }
		void SetTileWidth(uint32_t w) { m_TileWidth = w; }

		uint32_t GetSubTileCount() const { return m_SubTileCount; }
		void SetSubTileCount(uint32_t count) { m_SubTileCount = count; }

		const std::string& GetSourcePath() const { return m_SourcePath; }
		void SetSourcePath(const std::string& path) { m_SourcePath = path; }

	private:
		std::string m_SourcePath;
		Ref<StorageBuffer> m_Buffer;
		uint32_t m_TileWidth = 8;
		uint32_t m_SubTileCount = 0;
	};

}
