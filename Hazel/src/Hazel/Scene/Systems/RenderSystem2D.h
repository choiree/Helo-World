#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class RenderSystem2D : public System
	{
	public:
		const char* Name() const override { return "RenderSystem2D"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Reads() const override {
			return {"TransformComponent", "SpriteRendererComponent", "CircleRendererComponent",
			        "TextComponent", "TileMapComponent", "SceneRenderContext"};
		}
	};

}
