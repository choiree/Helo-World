#pragma once

#include "Hazel/Scene/System.h"

#include <string>

namespace Hazel {

	struct SpriteAnimationComponent;

	class AnimationSystem : public System
	{
	public:
		const char* Name() const override { return "AnimationSystem"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Reads()  const override { return {"SpriteAnimationComponent", "SpriteRendererComponent"}; }
		std::vector<const char*> Writes() const override { return {"SpriteAnimationComponent", "SpriteRendererComponent"}; }

		static void Play(SpriteAnimationComponent& anim, const std::string& name);
		static void Stop(SpriteAnimationComponent& anim);
		static void Pause(SpriteAnimationComponent& anim);
		static void Resume(SpriteAnimationComponent& anim);
	};

}
