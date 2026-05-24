#include "hzpch.h"
#include "SystemGraph.h"

#include "Hazel/Core/Log.h"

namespace Hazel {

	void SystemGraph::Build()
	{
		// Sort each stage by priority (lower Priority runs first, like Unity's ScriptExecutionOrder)
		for (auto& stage : m_SystemsByStage)
		{
			std::sort(stage.begin(), stage.end(), [](System* a, System* b) {
				return a->Priority() < b->Priority();
			});
		}

		m_NeedsRebuild = false;
	}

	void SystemGraph::ExecuteStage(SystemStage stage, entt::registry& registry, Timestep ts)
	{
		if (m_NeedsRebuild)
			Build();

		for (auto* system : m_SystemsByStage[(size_t)stage])
		{
			HZ_PROFILE_SCOPE(system->Name());
			system->Execute(registry, ts);
		}
	}

	void SystemGraph::Execute(entt::registry& registry, Timestep ts)
	{
		for (size_t i = 0; i < (size_t)SystemStage::COUNT; i++)
			ExecuteStage((SystemStage)i, registry, ts);
	}

	void SystemGraph::OnAttach(entt::registry& registry)
	{
		if (m_NeedsRebuild)
			Build();

		for (auto& sys : m_Systems)
			sys->OnAttach(registry);
	}

	void SystemGraph::OnDetach(entt::registry& registry)
	{
		for (auto& sys : m_Systems)
			sys->OnDetach(registry);
	}

}
