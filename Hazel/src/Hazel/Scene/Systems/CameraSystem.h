#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class CameraSystem : public System
	{
	public:
		const char* Name() const override { return "CameraSystem"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Reads()  const override { return {"CameraComponent", "TransformComponent"}; }
		std::vector<const char*> Writes() const override { return {"SceneRenderContext"}; }
	};

}
