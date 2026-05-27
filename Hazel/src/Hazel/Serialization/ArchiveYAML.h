#pragma once

#include "Hazel/Serialization/Archive.h"

#include <yaml-cpp/yaml.h>
#include <vector>
#include <string>

namespace Hazel {

	class ArchiveYAML : public Archive
	{
	public:
		// Write mode — wraps an emitter
		explicit ArchiveYAML(YAML::Emitter& emitter);
		// Read mode — wraps a YAML node tree
		explicit ArchiveYAML(const YAML::Node& root);

		Mode GetMode() const override;

		void BeginObject(std::string_view name) override;
		void EndObject() override;
		void BeginArray(std::string_view name) override;
		void EndArray() override;
		bool HasKey(std::string_view name) override;

		void Value(std::string_view name, int32_t& v) override;
		void Value(std::string_view name, uint32_t& v) override;
		void Value(std::string_view name, int64_t& v) override;
		void Value(std::string_view name, uint64_t& v) override;
		void Value(std::string_view name, float& v) override;
		void Value(std::string_view name, double& v) override;
		void Value(std::string_view name, bool& v) override;
		void Value(std::string_view name, std::string& v) override;
		void Value(std::string_view name, glm::vec2& v) override;
		void Value(std::string_view name, glm::vec3& v) override;
		void Value(std::string_view name, glm::vec4& v) override;
		void Value(std::string_view name, UUID& v) override;

		// Read-mode sequence helpers
		uint32_t ElementCount() const;
		void Element(uint32_t index);

	private:
		Mode m_Mode;
		YAML::Emitter* m_Emitter = nullptr;
		YAML::Node m_Root;
		std::vector<YAML::Node> m_NodeStack;

		void PushNode(const YAML::Node& node);
		void PopNode();
		const YAML::Node& CurrentNode() const;
	};

}
