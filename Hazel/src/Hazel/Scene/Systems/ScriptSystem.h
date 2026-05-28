#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		explicit ScriptSystem(Scene* scene) : m_Scene(scene) {}

		const char* Name() const override { return "ScriptSystem"; }

		void Execute(entt::registry& registry, Timestep ts) override;

		std::vector<const char*> Reads()  const override { return {"ScriptComponent", "NativeScriptComponent"}; }
		std::vector<const char*> Writes() const override { return {"ScriptComponent", "NativeScriptComponent"}; }

	private:
		Scene* m_Scene;
	};

}
