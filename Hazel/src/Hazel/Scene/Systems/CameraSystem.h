#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class CameraSystem : public System
	{
	public:
		const char* Name() const override { return "CameraSystem"; }
		SystemStage Stage() const override { return SystemStage::PreRender; }

		void Execute(entt::registry& registry, Timestep ts) override;
	};

}
