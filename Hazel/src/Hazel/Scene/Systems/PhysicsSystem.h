#pragma once

#include "Hazel/Scene/System.h"
#include "box2d/box2d.h"

namespace Hazel {

	class PhysicsSystem : public System
	{
	public:
		explicit PhysicsSystem(b2WorldId& worldId) : m_WorldId(worldId) {}

		const char* Name() const override { return "PhysicsSystem"; }
		SystemStage Stage() const override { return SystemStage::Physics; }

		void Execute(entt::registry& registry, Timestep ts) override;

	private:
		b2WorldId& m_WorldId;
	};

}
