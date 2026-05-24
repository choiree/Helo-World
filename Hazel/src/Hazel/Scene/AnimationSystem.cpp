#include "hzpch.h"
#include "AnimationSystem.h"

#include "Components.h"

#include <entt/entt.hpp>

namespace Hazel {

	void AnimationSystem::Play(SpriteAnimationComponent& anim, const std::string& name)
	{
		auto it = anim.Clips.find(name);
		if (it != anim.Clips.end())
		{
			anim.CurrentClip = it->second;
			anim.CurrentFrame = 0;
			anim.FrameTimer = 0.0f;
			anim.Playing = true;
			anim.Finished = false;
		}
	}

	void AnimationSystem::Stop(SpriteAnimationComponent& anim)
	{
		anim.Playing = false;
		anim.CurrentFrame = 0;
		anim.FrameTimer = 0.0f;
		anim.Finished = false;
	}

	void AnimationSystem::Pause(SpriteAnimationComponent& anim)
	{
		anim.Playing = false;
	}

	void AnimationSystem::Resume(SpriteAnimationComponent& anim)
	{
		anim.Playing = true;
	}

	void AnimationSystem::Execute(entt::registry& registry, Timestep ts)
	{
		auto view = registry.view<SpriteRendererComponent, SpriteAnimationComponent>();
		for (auto [entity, sprite, anim] : view.each())
		{
			if (!anim.Playing || !anim.CurrentClip)
				continue;

			size_t frameCount = anim.CurrentClip->GetFrameCount();
			if (frameCount == 0)
				continue;

			const auto& frame = anim.CurrentClip->GetFrame(anim.CurrentFrame);

			anim.FrameTimer += (float)ts * anim.SpeedMultiplier;
			if (anim.FrameTimer >= frame.Duration)
			{
				anim.FrameTimer = 0.0f;
				anim.CurrentFrame++;

				if ((size_t)anim.CurrentFrame >= frameCount)
				{
					if (anim.CurrentClip->IsLooping())
					{
						anim.CurrentFrame = 0;
					}
					else
					{
						anim.Finished = true;
						if (anim.OnFinished)
							anim.OnFinished();
						Stop(anim);
						continue;
					}
				}
			}

			const auto& currentFrame = anim.CurrentClip->GetFrame(anim.CurrentFrame);
			if (currentFrame.SubTexture)
				sprite.SubTexture = currentFrame.SubTexture;
		}
	}

}
