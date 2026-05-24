#pragma once

#include "Hazel/Scene/System.h"

#include <string>

namespace Hazel {

	struct SpriteAnimationComponent;

	class AnimationSystem : public System
	{
	public:
		const char* Name() const override { return "AnimationSystem"; }
		SystemStage Stage() const override { return SystemStage::Update; }

		void Execute(entt::registry& registry, Timestep ts) override;

		// Utility helpers (called from editor, test code — operate on a single component)
		static void Play(SpriteAnimationComponent& anim, const std::string& name);
		static void Stop(SpriteAnimationComponent& anim);
		static void Pause(SpriteAnimationComponent& anim);
		static void Resume(SpriteAnimationComponent& anim);
	};

}
