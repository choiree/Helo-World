#include "hzpch.h"
#include "Hazel/Renderer/Renderer2D.h"

#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/UniformBuffer.h"
#include "Hazel/Renderer/RenderCommand.h"
#include "Hazel/Renderer/TileMapAsset.h"
#include "Hazel/Renderer/TileSetAsset.h"
#include "Hazel/Renderer/PaletteAsset.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "MSDFData.h"

namespace Hazel {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		// Editor-only
		int EntityID;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		glm::vec3 LocalPosition;
		glm::vec4 Color;
		float Thickness;
		float Fade;

		// Editor-only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;

		// TODO: bg color for outline/bg

		// Editor-only
		int EntityID;
	};

	struct TileInstance
	{
		int32_t GridX;
		int32_t GridY;
		int32_t PackedData;
		int32_t EntityID;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		Ref<VertexArray> QuadVertexArray;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<Shader> QuadShader;
		Ref<Texture2D> WhiteTexture;

		Ref<VertexArray> CircleVertexArray;
		Ref<VertexBuffer> CircleVertexBuffer;
		Ref<Shader> CircleShader;

		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<Shader> LineShader;

		Ref<VertexArray> TextVertexArray;
		Ref<VertexBuffer> TextVertexBuffer;
		Ref<Shader> TextShader;

		// TileMap
		static const uint32_t MaxTileInstances = 16384; // 64×64 map × 4 subtiles per tile
		Ref<VertexArray> TileVertexArray;
		Ref<VertexBuffer> TileInstanceVBO;
		Ref<Shader> TileShader;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		uint32_t TextIndexCount = 0;
		TextVertex* TextVertexBufferBase = nullptr;
		TextVertex* TextVertexBufferPtr = nullptr;

		uint32_t TileInstanceCount = 0;
		TileInstance* TileInstanceBufferBase = nullptr;
		TileInstance* TileInstanceBufferPtr = nullptr;

		float LineWidth = 2.0f;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		Ref<Texture2D> FontAtlasTexture;

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		Ref<UniformBuffer> CameraUniformBuffer;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		HZ_PROFILE_FUNCTION();

		s_Data.QuadVertexArray = VertexArray::Create();

		s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));
		s_Data.QuadVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Float,  "a_TexIndex"     },
			{ ShaderDataType::Float,  "a_TilingFactor" },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_Data.QuadVertexArray->AddVertexBuffer(s_Data.QuadVertexBuffer);

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
		{
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
		s_Data.QuadVertexArray->SetIndexBuffer(quadIB);
		delete[] quadIndices;

		// Circles
		s_Data.CircleVertexArray = VertexArray::Create();

		s_Data.CircleVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(CircleVertex));
		s_Data.CircleVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_WorldPosition" },
			{ ShaderDataType::Float3, "a_LocalPosition" },
			{ ShaderDataType::Float4, "a_Color"         },
			{ ShaderDataType::Float,  "a_Thickness"     },
			{ ShaderDataType::Float,  "a_Fade"          },
			{ ShaderDataType::Int,    "a_EntityID"      }
		});
		s_Data.CircleVertexArray->AddVertexBuffer(s_Data.CircleVertexBuffer);
		s_Data.CircleVertexArray->SetIndexBuffer(quadIB); // Use quad IB
		s_Data.CircleVertexBufferBase = new CircleVertex[s_Data.MaxVertices];

		// Lines
		s_Data.LineVertexArray = VertexArray::Create();

		s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(LineVertex));
		s_Data.LineVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color"    },
			{ ShaderDataType::Int,    "a_EntityID" }
		});
		s_Data.LineVertexArray->AddVertexBuffer(s_Data.LineVertexBuffer);
		s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxVertices];

		// Text
		s_Data.TextVertexArray = VertexArray::Create();

		s_Data.TextVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(TextVertex));
		s_Data.TextVertexBuffer->SetLayout({
			{ ShaderDataType::Float3, "a_Position"     },
			{ ShaderDataType::Float4, "a_Color"        },
			{ ShaderDataType::Float2, "a_TexCoord"     },
			{ ShaderDataType::Int,    "a_EntityID"     }
		});
		s_Data.TextVertexArray->AddVertexBuffer(s_Data.TextVertexBuffer);
		s_Data.TextVertexArray->SetIndexBuffer(quadIB);
		s_Data.TextVertexBufferBase = new TextVertex[s_Data.MaxVertices];

		// TileMap
		{
			// Fixed template VBO: 4 vertices, just texcoords
			float templateVertices[] = {
				0.0f, 0.0f,
				1.0f, 0.0f,
				1.0f, 1.0f,
				0.0f, 1.0f
			};
			Ref<VertexBuffer> templateVB = VertexBuffer::Create(templateVertices, sizeof(templateVertices));
			templateVB->SetLayout({
				{ ShaderDataType::Float2, "a_TexCoord" }
			});

			// Instance VBO: per-subtile data (16 bytes each)
			s_Data.TileInstanceVBO = VertexBuffer::Create(s_Data.MaxTileInstances * sizeof(TileInstance));
			s_Data.TileInstanceVBO->SetLayout({
				{ ShaderDataType::Int, "a_GridX",      false, 1 },
				{ ShaderDataType::Int, "a_GridY",      false, 1 },
				{ ShaderDataType::Int, "a_PackedData", false, 1 },
				{ ShaderDataType::Int, "a_EntityID",   false, 1 }
			});

			// Index buffer: 6 indices for the template quad
			uint32_t tileIndices[] = { 0, 1, 2, 2, 3, 0 };
			Ref<IndexBuffer> tileIB = IndexBuffer::Create(tileIndices, 6);

			s_Data.TileVertexArray = VertexArray::Create();
			s_Data.TileVertexArray->AddVertexBuffer(templateVB);
			s_Data.TileVertexArray->AddVertexBuffer(s_Data.TileInstanceVBO);
			s_Data.TileVertexArray->SetIndexBuffer(tileIB);
			s_Data.TileInstanceBufferBase = new TileInstance[s_Data.MaxTileInstances];
		}

		s_Data.WhiteTexture = Texture2D::Create(TextureSpecification());
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
			samplers[i] = i;

		s_Data.QuadShader = Shader::Create("assets/shaders/Renderer2D_Quad.glsl");
		s_Data.CircleShader = Shader::Create("assets/shaders/Renderer2D_Circle.glsl");
		s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");
		s_Data.TextShader = Shader::Create("assets/shaders/Renderer2D_Text.glsl");
		s_Data.TileShader = Shader::Create("assets/shaders/TileMap.glsl");

		// Set first texture slot to 0
		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);
	}

	void Renderer2D::Shutdown()
	{
		HZ_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		HZ_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjectionMatrix();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		HZ_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		HZ_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_Data.CameraUniformBuffer->SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		HZ_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.CircleIndexCount = 0;
		s_Data.CircleVertexBufferPtr = s_Data.CircleVertexBufferBase;

		s_Data.LineVertexCount = 0;
		s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;

		s_Data.TextIndexCount = 0;
		s_Data.TextVertexBufferPtr = s_Data.TextVertexBufferBase;

		s_Data.TileInstanceCount = 0;
		s_Data.TileInstanceBufferPtr = s_Data.TileInstanceBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::Flush()
	{
		if (s_Data.QuadIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
			s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

			// Bind textures
			for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
				s_Data.TextureSlots[i]->Bind(i);

			s_Data.QuadShader->Bind();
			RenderCommand::DrawIndexed(s_Data.QuadVertexArray, s_Data.QuadIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.CircleIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.CircleVertexBufferPtr - (uint8_t*)s_Data.CircleVertexBufferBase);
			s_Data.CircleVertexBuffer->SetData(s_Data.CircleVertexBufferBase, dataSize);

			s_Data.CircleShader->Bind();
			RenderCommand::DrawIndexed(s_Data.CircleVertexArray, s_Data.CircleIndexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.LineVertexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.LineVertexBufferPtr - (uint8_t*)s_Data.LineVertexBufferBase);
			s_Data.LineVertexBuffer->SetData(s_Data.LineVertexBufferBase, dataSize);

			s_Data.LineShader->Bind();
			RenderCommand::SetLineWidth(s_Data.LineWidth);
			RenderCommand::DrawLines(s_Data.LineVertexArray, s_Data.LineVertexCount);
			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.TextIndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TextVertexBufferPtr - (uint8_t*)s_Data.TextVertexBufferBase);
			s_Data.TextVertexBuffer->SetData(s_Data.TextVertexBufferBase, dataSize);

			auto buf = s_Data.TextVertexBufferBase;
			s_Data.FontAtlasTexture->Bind(0);

			s_Data.TextShader->Bind();
			RenderCommand::DrawIndexed(s_Data.TextVertexArray, s_Data.TextIndexCount);
			s_Data.Stats.DrawCalls++;
		}
	}

	void Renderer2D::NextBatch()
	{
		Flush();
		StartBatch();
	}

	static void DrawTileLayer(const TileMapLayer& layer, const Ref<TileSetAsset>& tileset,
		uint32_t mapWidth, int entityID)
	{
		if (!tileset)
			return;

		constexpr int32_t subOffX[4] = { 0, 8, 0, 8 };
		constexpr int32_t subOffY[4] = { 8, 8, 0, 0 };

		for (uint32_t cell = 0; cell < (uint32_t)layer.MapData.size(); cell++)
		{
			uint16_t tileIndex = layer.MapData[cell];
			if (tileIndex == 0 || tileIndex >= layer.TileIndices.size())
				continue;

			uint16_t subBase = layer.TileIndices[tileIndex];

			int32_t tileX = int32_t(cell % mapWidth) * 16;
			int32_t tileY = int32_t(cell / mapWidth) * 16;

			for (int slot = 0; slot < 4; slot++)
			{
				uint32_t idx = subBase + slot;
				if (idx >= layer.SubTiles.size())
					continue;

				uint32_t packed = layer.SubTiles[idx];
				if (packed == 0)
					continue;

				s_Data.TileInstanceBufferPtr->GridX = tileX + subOffX[slot];
				s_Data.TileInstanceBufferPtr->GridY = tileY + subOffY[slot];
				s_Data.TileInstanceBufferPtr->PackedData = static_cast<int32_t>(packed);
				s_Data.TileInstanceBufferPtr->EntityID = entityID;
				s_Data.TileInstanceBufferPtr++;
				s_Data.TileInstanceCount++;
				s_Data.Stats.QuadCount++;
			}
		}
	}

	void Renderer2D::DrawTileMap(const glm::mat4& transform, const Ref<TileMapAsset>& map,
		const Ref<TileSetAsset>& bottomTileSet, const Ref<TileSetAsset>& topTileSet,
		const Ref<PaletteAsset>& palette, int entityID)
	{
		if (!map)
			return;

		uint32_t startInstanceCount = s_Data.TileInstanceCount;

		uint32_t mapW = map->GetWidth();
		DrawTileLayer(map->Bottom, bottomTileSet, mapW, entityID);
		DrawTileLayer(map->Top, topTileSet, mapW, entityID);

		if (s_Data.TileInstanceCount == startInstanceCount)
			return;

		// Bind SSBOs
		if (bottomTileSet)
			bottomTileSet->GetBuffer()->Bind(1);
		if (palette)
			palette->GetBuffer()->Bind(2);

		// Upload instance buffer
		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.TileInstanceBufferPtr - (uint8_t*)s_Data.TileInstanceBufferBase);
		s_Data.TileInstanceVBO->SetData(s_Data.TileInstanceBufferBase, dataSize);

		s_Data.TileShader->Bind();
		s_Data.TileShader->SetMat4("u_Model", transform);
		RenderCommand::DrawIndexedInstanced(s_Data.TileVertexArray, 6, s_Data.TileInstanceCount);
		s_Data.Stats.DrawCalls++;

		// Reset for next entity
		s_Data.TileInstanceCount = 0;
		s_Data.TileInstanceBufferPtr = s_Data.TileInstanceBufferBase;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		HZ_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		HZ_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		HZ_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		constexpr glm::vec2 defaultTexCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		DrawQuad(transform, texture, defaultTexCoords, tilingFactor, tintColor, entityID);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, const glm::vec2* texCoords, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		HZ_PROFILE_FUNCTION();

		constexpr size_t quadVertexCount = 4;

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i] == *texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		HZ_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		HZ_PROFILE_FUNCTION();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawCircle(const glm::mat4& transform, const glm::vec4& color, float thickness /*= 1.0f*/, float fade /*= 0.005f*/, int entityID /*= -1*/)
	{
		HZ_PROFILE_FUNCTION();

		if (s_Data.CircleIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 4; i++)
		{
			s_Data.CircleVertexBufferPtr->WorldPosition = transform * s_Data.QuadVertexPositions[i];
			s_Data.CircleVertexBufferPtr->LocalPosition = s_Data.QuadVertexPositions[i] * 2.0f;
			s_Data.CircleVertexBufferPtr->Color = color;
			s_Data.CircleVertexBufferPtr->Thickness = thickness;
			s_Data.CircleVertexBufferPtr->Fade = fade;
			s_Data.CircleVertexBufferPtr->EntityID = entityID;
			s_Data.CircleVertexBufferPtr++;
		}

		s_Data.CircleIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawLine(const glm::vec3& p0, glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		s_Data.LineVertexBufferPtr->Position = p0;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexBufferPtr->Position = p1;
		s_Data.LineVertexBufferPtr->Color = color;
		s_Data.LineVertexBufferPtr->EntityID = entityID;
		s_Data.LineVertexBufferPtr++;

		s_Data.LineVertexCount += 2;
	}

	void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int entityID)
	{
		glm::vec3 p0 = glm::vec3(position.x - size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p1 = glm::vec3(position.x + size.x * 0.5f, position.y - size.y * 0.5f, position.z);
		glm::vec3 p2 = glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, position.z);
		glm::vec3 p3 = glm::vec3(position.x - size.x * 0.5f, position.y + size.y * 0.5f, position.z);

		DrawLine(p0, p1, color, entityID);
		DrawLine(p1, p2, color, entityID);
		DrawLine(p2, p3, color, entityID);
		DrawLine(p3, p0, color, entityID);
	}

	void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		glm::vec3 lineVertices[4];
		for (size_t i = 0; i < 4; i++)
			lineVertices[i] = transform * s_Data.QuadVertexPositions[i];

		DrawLine(lineVertices[0], lineVertices[1], color, entityID);
		DrawLine(lineVertices[1], lineVertices[2], color, entityID);
		DrawLine(lineVertices[2], lineVertices[3], color, entityID);
		DrawLine(lineVertices[3], lineVertices[0], color, entityID);
	}

	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
	{
		if (src.Texture)
		{
			if (src.SubTexture)
			{
				const glm::vec2& uv0 = src.SubTexture->GetUV0();
				const glm::vec2& uv1 = src.SubTexture->GetUV1();
				const glm::vec2 texCoords[] = {
					{ uv0.x, uv0.y },
					{ uv1.x, uv0.y },
					{ uv1.x, uv1.y },
					{ uv0.x, uv1.y }
				};
				DrawQuad(transform, src.Texture, texCoords, src.TilingFactor, src.Color, entityID);
			}
			else
			{
				DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, entityID);
			}
		}
		else
			DrawQuad(transform, src.Color, entityID);
	}

	void Renderer2D::DrawString(const std::string& string, Ref<Font> font, const glm::mat4& transform, const TextParams& textParams, int entityID)
	{
		//const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
		//const auto& metrics = fontGeometry.getMetrics();
		//Ref<Texture2D> fontAtlas = font->GetAtlasTexture();

		//s_Data.FontAtlasTexture = fontAtlas;

		//double x = 0.0;
		//double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
		//double y = 0.0;

		//const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();
		//
		//for (size_t i = 0; i < string.size(); i++)
		//{
		//	char character = string[i];
		//	if (character == '\r')
		//		continue;

		//	if (character == '\n')
		//	{
		//		x = 0;
		//		y -= fsScale * metrics.lineHeight + textParams.LineSpacing;
		//		continue;
		//	}

		//	if (character == ' ')
		//	{
		//		float advance = spaceGlyphAdvance;
		//		if (i < string.size() - 1)
		//		{
		//			char nextCharacter = string[i + 1];
		//			double dAdvance;
		//			fontGeometry.getAdvance(dAdvance, character, nextCharacter);
		//			advance = (float)dAdvance;
		//		}

		//		x += fsScale * advance + textParams.Kerning;
		//		continue;
		//	}

		//	if (character == '\t')
		//	{
		//		// NOTE(Yan): is this right?
		//		x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
		//		continue;
		//	}

		//	auto glyph = fontGeometry.getGlyph(character);
		//	if (!glyph)
		//		glyph = fontGeometry.getGlyph('?');
		//	if (!glyph)
		//		return;

		//	double al, ab, ar, at;
		//	glyph->getQuadAtlasBounds(al, ab, ar, at);
		//	glm::vec2 texCoordMin((float)al, (float)ab);
		//	glm::vec2 texCoordMax((float)ar, (float)at);

		//	double pl, pb, pr, pt;
		//	glyph->getQuadPlaneBounds(pl, pb, pr, pt);
		//	glm::vec2 quadMin((float)pl, (float)pb);
		//	glm::vec2 quadMax((float)pr, (float)pt);

		//	quadMin *= fsScale, quadMax *= fsScale;
		//	quadMin += glm::vec2(x, y);
		//	quadMax += glm::vec2(x, y);

		//	float texelWidth = 1.0f / fontAtlas->GetWidth();
		//	float texelHeight = 1.0f / fontAtlas->GetHeight();
		//	texCoordMin *= glm::vec2(texelWidth, texelHeight);
		//	texCoordMax *= glm::vec2(texelWidth, texelHeight);

		//	// render here
		//	s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin, 0.0f, 1.0f);
		//	s_Data.TextVertexBufferPtr->Color = textParams.Color;
		//	s_Data.TextVertexBufferPtr->TexCoord = texCoordMin;
		//	s_Data.TextVertexBufferPtr->EntityID = entityID;
		//	s_Data.TextVertexBufferPtr++;

		//	s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f);
		//	s_Data.TextVertexBufferPtr->Color = textParams.Color;
		//	s_Data.TextVertexBufferPtr->TexCoord = { texCoordMin.x, texCoordMax.y };
		//	s_Data.TextVertexBufferPtr->EntityID = entityID;
		//	s_Data.TextVertexBufferPtr++;

		//	s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax, 0.0f, 1.0f);
		//	s_Data.TextVertexBufferPtr->Color = textParams.Color;
		//	s_Data.TextVertexBufferPtr->TexCoord = texCoordMax;
		//	s_Data.TextVertexBufferPtr->EntityID = entityID;
		//	s_Data.TextVertexBufferPtr++;

		//	s_Data.TextVertexBufferPtr->Position = transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f);
		//	s_Data.TextVertexBufferPtr->Color = textParams.Color;
		//	s_Data.TextVertexBufferPtr->TexCoord = { texCoordMax.x, texCoordMin.y };
		//	s_Data.TextVertexBufferPtr->EntityID = entityID;
		//	s_Data.TextVertexBufferPtr++;

		//	s_Data.TextIndexCount += 6;
		//	s_Data.Stats.QuadCount++;

		//	if (i < string.size() - 1)
		//	{
		//		double advance = glyph->getAdvance();
		//		char nextCharacter = string[i + 1];
		//		fontGeometry.getAdvance(advance, character, nextCharacter);

		//		x += fsScale * advance + textParams.Kerning;
		//	}
		//}
	}

	void Renderer2D::DrawString(const std::string& string, const glm::mat4& transform, const TextComponent& component, int entityID)
	{
		// todo 待实现
		// DrawString(string, component.FontAsset, transform, { component.Color, component.Kerning, component.LineSpacing }, entityID);
	}

	float Renderer2D::GetLineWidth()
	{
		return s_Data.LineWidth;
	}

	void Renderer2D::SetLineWidth(float width)
	{
		s_Data.LineWidth = width;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}

}
