#include "hzpch.h"
#include "ArchiveYAML.h"

namespace Hazel {

	// ---- Write mode ----

	ArchiveYAML::ArchiveYAML(YAML::Emitter& emitter)
		: m_Mode(Mode::Write), m_Emitter(&emitter)
	{
	}

	// ---- Read mode ----

	ArchiveYAML::ArchiveYAML(const YAML::Node& root)
		: m_Mode(Mode::Read), m_Root(root)
	{
	}

	Archive::Mode ArchiveYAML::GetMode() const
	{
		return m_Mode;
	}

	// ---- Node stack helpers ----

	void ArchiveYAML::PushNode(const YAML::Node& node)
	{
		m_NodeStack.push_back(node);
	}

	void ArchiveYAML::PopNode()
	{
		if (!m_NodeStack.empty())
			m_NodeStack.pop_back();
	}

	const YAML::Node& ArchiveYAML::CurrentNode() const
	{
		return m_NodeStack.empty() ? m_Root : m_NodeStack.back();
	}

	// ---- BeginObject / EndObject ----

	void ArchiveYAML::BeginObject(std::string_view name)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << YAML::BeginMap;
		}
		else
		{
			const auto& cur = CurrentNode();
			if (cur[std::string(name)])
				PushNode(cur[std::string(name)]);
		}
	}

	void ArchiveYAML::EndObject()
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::EndMap;
		}
		else
		{
			PopNode();
		}
	}

	// ---- BeginArray / EndArray ----

	void ArchiveYAML::BeginArray(std::string_view name)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << YAML::BeginSeq;
		}
		else
		{
			const auto& cur = CurrentNode();
			if (cur[std::string(name)])
				PushNode(cur[std::string(name)]);
		}
	}

	void ArchiveYAML::EndArray()
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::EndSeq;
		}
		else
		{
			PopNode();
		}
	}

	// ---- HasKey ----

	bool ArchiveYAML::HasKey(std::string_view name)
	{
		if (m_Mode == Mode::Write)
			return false;
		return CurrentNode()[std::string(name)].IsDefined();
	}

	// ---- Sequence helpers (read mode) ----

	uint32_t ArchiveYAML::ElementCount() const
	{
		const auto& cur = CurrentNode();
		if (cur.IsSequence())
			return static_cast<uint32_t>(cur.size());
		return 0;
	}

	void ArchiveYAML::Element(uint32_t index)
	{
		const auto& cur = CurrentNode();
		if (cur.IsSequence() && index < cur.size())
			PushNode(cur[index]);
	}

	// ---- Scalar Value overloads (read helpers) ----

	template<typename T>
	static T ReadScalar(const YAML::Node& node, std::string_view name, const T& fallback)
	{
		auto child = node[std::string(name)];
		if (child)
			return child.as<T>(fallback);
		return fallback;
	}

	// ---- Value implementations ----

	void ArchiveYAML::Value(std::string_view name, int32_t& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<int32_t>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, uint32_t& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<uint32_t>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, int64_t& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<int64_t>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, uint64_t& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<uint64_t>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, float& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<float>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, double& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<double>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, bool& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<bool>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, std::string& v)
	{
		if (m_Mode == Mode::Write)
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << v;
		else
			v = ReadScalar<std::string>(CurrentNode(), name, v);
	}

	void ArchiveYAML::Value(std::string_view name, glm::vec2& v)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value
				<< YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		}
		else
		{
			auto child = CurrentNode()[std::string(name)];
			if (child && child.IsSequence() && child.size() >= 2)
			{
				v.x = child[0].as<float>();
				v.y = child[1].as<float>();
			}
		}
	}

	void ArchiveYAML::Value(std::string_view name, glm::vec3& v)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value
				<< YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		}
		else
		{
			auto child = CurrentNode()[std::string(name)];
			if (child && child.IsSequence() && child.size() >= 3)
			{
				v.x = child[0].as<float>();
				v.y = child[1].as<float>();
				v.z = child[2].as<float>();
			}
		}
	}

	void ArchiveYAML::Value(std::string_view name, glm::vec4& v)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value
				<< YAML::Flow << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		}
		else
		{
			auto child = CurrentNode()[std::string(name)];
			if (child && child.IsSequence() && child.size() >= 4)
			{
				v.x = child[0].as<float>();
				v.y = child[1].as<float>();
				v.z = child[2].as<float>();
				v.w = child[3].as<float>();
			}
		}
	}

	void ArchiveYAML::Value(std::string_view name, UUID& v)
	{
		if (m_Mode == Mode::Write)
		{
			*m_Emitter << YAML::Key << std::string(name) << YAML::Value << (uint64_t)v;
		}
		else
		{
			auto child = CurrentNode()[std::string(name)];
			if (child)
				v = child.as<uint64_t>((uint64_t)v);
		}
	}

}
