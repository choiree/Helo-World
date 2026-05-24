#pragma once

#include "Hazel/Scene/System.h"

#include <array>
#include <vector>

namespace Hazel {

	class SystemGraph
	{
	public:
		SystemGraph() = default;

		template<typename T, typename... Args>
		T* AddSystem(Args&&... args)
		{
			Ref<T> system = CreateRef<T>(std::forward<Args>(args)...);
			T* ptr = system.get();
			m_Systems.push_back(system);
			m_SystemsByStage[(size_t)ptr->Stage()].push_back(ptr);
			m_NeedsRebuild = true;
			return ptr;
		}

		Ref<System> GetSystem(const char* name)
		{
			for (auto& sys : m_Systems)
				if (strcmp(sys->Name(), name) == 0)
					return sys;
			return nullptr;
		}

		void Build();

		void ExecuteStage(SystemStage stage, entt::registry& registry, Timestep ts);

		void Execute(entt::registry& registry, Timestep ts);

		void OnAttach(entt::registry& registry);
		void OnDetach(entt::registry& registry);

	private:
		std::vector<Ref<System>> m_Systems;
		std::array<std::vector<System*>, (size_t)SystemStage::COUNT> m_SystemsByStage;
		bool m_NeedsRebuild = true;
	};

}
