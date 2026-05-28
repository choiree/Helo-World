#pragma once

#include "Hazel/Scene/System.h"

#include <array>
#include <string_view>
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

		void Execute(entt::registry& registry, Timestep ts);

		void Execute(entt::registry& registry, Timestep ts, std::initializer_list<const char*> filter);

		void OnAttach(entt::registry& registry);
		void OnDetach(entt::registry& registry);

		// Dump dependency graph in Graphviz dot format
		void DebugDump() const;

	private:
		std::vector<Ref<System>> m_Systems;
		std::vector<std::vector<System*>> m_Levels;
		bool m_NeedsRebuild = true;
	};

}
