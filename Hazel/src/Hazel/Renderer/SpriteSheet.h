#pragma once

#include "Texture.h"

#include <glm/glm.hpp>
#include <string>

namespace Hazel {

	class SpriteSheet
	{
	public:
		static Ref<SpriteSheet> Create()
		{
			return CreateRef<SpriteSheet>();
		}

		Ref<Texture2D> GetTexture() const { return m_Texture; }
		void SetTexture(const Ref<Texture2D>& tex) { m_Texture = tex; }

		uint32_t GetCellWidth() const { return m_CellWidth; }
		uint32_t GetCellHeight() const { return m_CellHeight; }
		void SetCellSize(uint32_t w, uint32_t h) { m_CellWidth = w; m_CellHeight = h; }

		uint32_t GetColumns() const
		{
			if (m_CellWidth == 0 || !m_Texture) return 0;
			return m_Texture->GetWidth() / m_CellWidth;
		}

		uint32_t GetRows() const
		{
			if (m_CellHeight == 0 || !m_Texture) return 0;
			return m_Texture->GetHeight() / m_CellHeight;
		}

		// Compute UV for cell at grid position (col, row)
		glm::vec2 CellToUV0(uint32_t col, uint32_t row) const
		{
			if (!m_Texture || m_CellWidth == 0 || m_CellHeight == 0)
				return { 0.0f, 0.0f };

			float texW = (float)m_Texture->GetWidth();
			float texH = (float)m_Texture->GetHeight();
			return { (col * m_CellWidth) / texW, (row * m_CellHeight) / texH };
		}

		glm::vec2 CellToUV1(uint32_t col, uint32_t row) const
		{
			if (!m_Texture || m_CellWidth == 0 || m_CellHeight == 0)
				return { 1.0f, 1.0f };

			float texW = (float)m_Texture->GetWidth();
			float texH = (float)m_Texture->GetHeight();
			return { ((col + 1) * m_CellWidth) / texW, ((row + 1) * m_CellHeight) / texH };
		}

		const std::string& GetSourcePath() const { return m_SourcePath; }
		void SetSourcePath(const std::string& path) { m_SourcePath = path; }

	private:
		std::string m_SourcePath;
		Ref<Texture2D> m_Texture;
		uint32_t m_CellWidth = 0;
		uint32_t m_CellHeight = 0;
	};

}
