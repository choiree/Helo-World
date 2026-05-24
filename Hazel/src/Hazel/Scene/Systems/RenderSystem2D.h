#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	// Handles both runtime (primary camera) and editor (EditorCamera) rendering.
	// Checks SceneRenderContext in registry.ctx() to decide which path to take.
	class RenderSystem2D : public System
	{
	public:
		const char* Name() const override { return "RenderSystem2D"; }
		SystemStage Stage() const override { return SystemStage::Render; }

		void Execute(entt::registry& registry, Timestep ts) override;
	};

}
