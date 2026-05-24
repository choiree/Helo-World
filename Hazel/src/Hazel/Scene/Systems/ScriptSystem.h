#pragma once

#include "Hazel/Scene/System.h"

namespace Hazel {

	class Scene;

	class ScriptSystem : public System
	{
	public:
		explicit ScriptSystem(Scene* scene) : m_Scene(scene) {}

		const char* Name() const override { return "ScriptSystem"; }
		SystemStage Stage() const override { return SystemStage::PreUpdate; }

		void Execute(entt::registry& registry, Timestep ts) override;

	private:
		Scene* m_Scene;
	};

}
