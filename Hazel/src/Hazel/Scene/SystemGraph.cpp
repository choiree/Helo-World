#include "hzpch.h"
#include "SystemGraph.h"

#include "Hazel/Core/Log.h"

#include <entt/graph/flow.hpp>
#include <entt/graph/dot.hpp>
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace Hazel {

	void SystemGraph::Build()
	{
		m_Levels.clear();

		if (m_Systems.empty())
		{
			m_NeedsRebuild = false;
			return;
		}

		entt::flow builder;
		std::unordered_map<entt::id_type, System*> idToSystem;

		for (auto& sys : m_Systems)
		{
			entt::id_type sid = entt::hashed_string{sys->Name()};
			idToSystem[sid] = sys.get();

			builder.bind(sid);

			auto reads  = sys->Reads();
			auto writes = sys->Writes();

			for (auto* r : reads)
				builder.ro(entt::hashed_string{r});
			for (auto* w : writes)
				builder.rw(entt::hashed_string{w});
		}

		auto matrix = builder.graph();
		const auto n = matrix.size();

		// vertex index -> id_type
		std::vector<entt::id_type> vertexToId(n);
		for (auto v : matrix.vertices())
			vertexToId[v] = builder[v];

		// Compute in-degree for each vertex
		std::vector<size_t> inDegree(n, 0);
		for (auto v : matrix.vertices())
		{
			for (auto [from, to] : matrix.in_edges(v))
				inDegree[v]++;
		}

		// Seed with zero-in-degree vertices
		std::vector<size_t> queue;
		for (size_t v = 0; v < n; ++v)
			if (inDegree[v] == 0)
				queue.push_back(v);

		// Kahn: each level = systems that can run in parallel
		while (!queue.empty())
		{
			std::vector<System*> level;
			level.reserve(queue.size());

			for (size_t v : queue)
			{
				auto it = idToSystem.find(vertexToId[v]);
				if (it != idToSystem.end())
					level.push_back(it->second);
			}

			if (!level.empty())
				m_Levels.push_back(std::move(level));

			std::vector<size_t> nextQueue;
			for (size_t v : queue)
			{
				for (auto [from, to] : matrix.out_edges(v))
				{
					inDegree[to]--;
					if (inDegree[to] == 0)
						nextQueue.push_back(to);
				}
			}
			queue = std::move(nextQueue);
		}

		m_NeedsRebuild = false;

		HZ_CORE_INFO("SystemGraph: {} system(s) in {} level(s)", m_Systems.size(), m_Levels.size());
	}

	void SystemGraph::Execute(entt::registry& registry, Timestep ts)
	{
		if (m_NeedsRebuild)
			Build();

		for (auto& level : m_Levels)
		{
			for (auto* system : level)
			{
				HZ_PROFILE_SCOPE(system->Name());
				system->Execute(registry, ts);
			}
		}
	}

	void SystemGraph::Execute(entt::registry& registry, Timestep ts, std::initializer_list<const char*> filter)
	{
		if (m_NeedsRebuild)
			Build();

		for (auto& level : m_Levels)
		{
			for (auto* system : level)
			{
				bool included = false;
				for (auto* name : filter)
				{
					if (strcmp(system->Name(), name) == 0)
					{
						included = true;
						break;
					}
				}
				if (!included)
					continue;

				HZ_PROFILE_SCOPE(system->Name());
				system->Execute(registry, ts);
			}
		}
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

	void SystemGraph::DebugDump() const
	{
		std::ostringstream oss;
		for (size_t i = 0; i < m_Levels.size(); ++i)
		{
			oss << "Level " << i << ": ";
			for (auto* sys : m_Levels[i])
				oss << sys->Name() << "  ";
			oss << "\n";
		}
		HZ_CORE_TRACE("SystemGraph:\n{}", oss.str());

		// Rebuild flow for dot
		entt::flow builder;
		for (auto& sys : m_Systems)
		{
			entt::id_type sid = entt::hashed_string{sys->Name()};
			builder.bind(sid);
			for (auto* r : sys->Reads())
				builder.ro(entt::hashed_string{r});
			for (auto* w : sys->Writes())
				builder.rw(entt::hashed_string{w});
		}

		auto matrix = builder.graph();
		std::ostringstream dot;
		entt::dot(dot, matrix, [this](std::ostream& out, size_t vertex) {
			out << "label=\"" << m_Systems[vertex]->Name() << "\"";
		});
		HZ_CORE_TRACE("SystemGraph dot:\n{}", dot.str());
	}

}
