#include "hzpch.h"
#include "ScriptSystem.h"

#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/ScriptableEntity.h"
#include "Hazel/Scripting/ScriptEngine.h"

namespace Hazel {

	void ScriptSystem::Execute(entt::registry& registry, Timestep ts)
	{
		// C# Entity OnUpdate
		auto view = registry.view<ScriptComponent>();
		for (auto [e, _] : view.each())
		{
			Entity entity = { e, m_Scene };
			ScriptEngine::OnUpdateEntity(entity, ts);
		}

		// Native script update
		{
			auto view = registry.view<NativeScriptComponent>();
			for (auto [e, nsc] : view.each())
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ e, m_Scene };
					nsc.Instance->OnCreate();
				}

				nsc.Instance->OnUpdate(ts);
			}
		}
	}

}
