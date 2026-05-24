#include "hzpch.h"
#include "TransformSyncSystem.h"

#include "Hazel/Scene/Components.h"

#include "box2d/box2d.h"

namespace Hazel {

	void TransformSyncSystem::Execute(entt::registry& registry, Timestep ts)
	{
		auto view = registry.view<TransformComponent, Rigidbody2DComponent>();
		for (auto [entity, transform, rb2d] : view.each())
		{
			b2BodyId bodyId = rb2d.RuntimeBody;

			if (!b2Body_IsValid(bodyId))
				continue;

			b2Vec2 position = b2Body_GetPosition(bodyId);
			b2Rot rotation = b2Body_GetRotation(bodyId);

			transform.Translation.x = position.x;
			transform.Translation.y = position.y;
			transform.Rotation.z = b2Rot_GetAngle(rotation);
		}
	}

}
