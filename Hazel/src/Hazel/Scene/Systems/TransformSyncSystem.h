#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class TransformSyncSystem : public System
	{
	public:
		const char* Name() const override { return "TransformSyncSystem"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Reads()  const override { return {"Rigidbody2DComponent"}; }
		std::vector<const char*> Writes() const override { return {"TransformComponent"}; }
	};

}
