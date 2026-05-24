#pragma once

#include "Hazel/Core/Timestep.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace Hazel {

	enum class SystemStage : uint8_t
	{
		PreUpdate,      // Input, script logic
		Update,         // Game logic, animation
		Physics,        // Physics step (single-threaded)
		PostPhysics,    // Transform sync, collision callbacks
		PreRender,      // Camera selection, culling
		Render,         // Draw command emission
		PostRender,     // UI, debug overlay
		COUNT
	};

	class System
	{
	public:
		virtual ~System() = default;

		virtual const char* Name() const = 0;
		virtual SystemStage Stage() const = 0;
		virtual int Priority() const { return 0; }

		// Lifecycle
		virtual void OnAttach(entt::registry& registry) {}
		virtual void OnDetach(entt::registry& registry) {}

		// Per-frame execution
		virtual void Execute(entt::registry& registry, Timestep ts) = 0;

		// Component access declarations (for future parallel scheduling)
		virtual std::vector<const char*> Reads()  const { return {}; }
		virtual std::vector<const char*> Writes() const { return {}; }
	};

}
