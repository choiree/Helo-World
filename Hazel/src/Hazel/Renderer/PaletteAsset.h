#pragma once

#include "Hazel/Renderer/StorageBuffer.h"

namespace Hazel {

	class PaletteAsset
	{
	public:
		static Ref<PaletteAsset> Create()
		{
			return CreateRef<PaletteAsset>();
		}

		Ref<StorageBuffer> GetBuffer() const { return m_Buffer; }
		void SetBuffer(const Ref<StorageBuffer>& buf) { m_Buffer = buf; }

		const std::string& GetSourcePath() const { return m_SourcePath; }
		void SetSourcePath(const std::string& path) { m_SourcePath = path; }

	private:
		std::string m_SourcePath;
		Ref<StorageBuffer> m_Buffer;
	};

}
