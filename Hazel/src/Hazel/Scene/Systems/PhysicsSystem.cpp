#include "hzpch.h"
#include "PhysicsSystem.h"

#include "Hazel/Scene/Components.h"

namespace Hazel {

	void PhysicsSystem::Execute(entt::registry& registry, Timestep ts)
	{
		const int32_t subStepCount = 4;
		b2World_Step(m_WorldId, ts, subStepCount);
	}

}
