#pragma once

#include "Texture.h"
#include <glm/glm.hpp>

namespace Hazel {

	class SubTexture2D
	{
	public:
		static Ref<SubTexture2D> Create(const Ref<Texture2D>& texture, const glm::vec2& uv0, const glm::vec2& uv1)
		{
			return CreateRef<SubTexture2D>(texture, uv0, uv1);
		}

		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& texture, const glm::vec2& cellPosition, const glm::vec2& cellSize)
		{
			glm::vec2 textureSize = { texture->GetWidth(), texture->GetHeight() };
			glm::vec2 uv0 = cellPosition / textureSize;
			glm::vec2 uv1 = (cellPosition + cellSize) / textureSize;
			return Create(texture, uv0, uv1);
		}

		SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& uv0, const glm::vec2& uv1)
			: m_Texture(texture), m_UV0(uv0), m_UV1(uv1) {}

		const Ref<Texture2D>& GetTexture() const { return m_Texture; }
		const glm::vec2& GetUV0() const { return m_UV0; }
		const glm::vec2& GetUV1() const { return m_UV1; }

	private:
		Ref<Texture2D> m_Texture;
		glm::vec2 m_UV0 = { 0.0f, 0.0f };
		glm::vec2 m_UV1 = { 1.0f, 1.0f };
	};

}
