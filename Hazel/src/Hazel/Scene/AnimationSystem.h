#pragma once

#include "Hazel/Core/Timestep.h"

#include <entt/entt.hpp>

#include <string>

namespace Hazel {

	struct SpriteAnimationComponent;

	class AnimationSystem
	{
	public:
		static void OnUpdate(entt::registry& registry, Timestep ts);

		static void Play(SpriteAnimationComponent& anim, const std::string& name);
		static void Stop(SpriteAnimationComponent& anim);
		static void Pause(SpriteAnimationComponent& anim);
		static void Resume(SpriteAnimationComponent& anim);
	};

}
