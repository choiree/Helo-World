#pragma once

#include "Hazel/Core/Timestep.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Hazel {

	class System
	{
	public:
		virtual ~System() = default;

		virtual const char* Name() const = 0;

		virtual void OnAttach(entt::registry& registry) {}
		virtual void OnDetach(entt::registry& registry) {}

		virtual void Execute(entt::registry& registry, Timestep ts) = 0;

		virtual std::vector<const char*> Reads()  const { return {}; }
		virtual std::vector<const char*> Writes() const { return {}; }
	};

}
