#pragma once

#include "Hazel/Renderer/SpriteSheet.h"

#include <string>

namespace Hazel {

	class SpriteSheetSerializer
	{
	public:
		static void Serialize(const std::string& filepath, const Ref<SpriteSheet>& sheet);
		static Ref<SpriteSheet> Deserialize(const std::string& filepath);
	};

}
