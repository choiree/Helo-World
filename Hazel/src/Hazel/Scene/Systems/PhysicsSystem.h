#pragma once

#include "Hazel/Scene/System.h"
#include "box2d/box2d.h"

namespace Hazel {

	class PhysicsSystem : public System
	{
	public:
		explicit PhysicsSystem(b2WorldId& worldId) : m_WorldId(worldId) {}

		const char* Name() const override { return "PhysicsSystem"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Writes() const override {
			return {"Rigidbody2DComponent", "BoxCollider2DComponent", "CircleCollider2DComponent"};
		}

	private:
		b2WorldId& m_WorldId;
	};

}
