#pragma once

#include "Hazel/Renderer/AnimationClip.h"

#include <string>

namespace Hazel {

	class AnimationClipSerializer
	{
	public:
		static void Serialize(const std::string& filepath, const Ref<AnimationClip>& clip);
		static Ref<AnimationClip> Deserialize(const std::string& filepath);
	};

}
