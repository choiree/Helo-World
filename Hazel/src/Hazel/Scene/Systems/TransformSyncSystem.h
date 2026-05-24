#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class TransformSyncSystem : public System
	{
	public:
		const char* Name() const override { return "TransformSyncSystem"; }
		SystemStage Stage() const override { return SystemStage::PostPhysics; }

		void Execute(entt::registry& registry, Timestep ts) override;
	};

}
