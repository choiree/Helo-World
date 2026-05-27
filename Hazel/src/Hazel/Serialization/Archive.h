#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/UUID.h"

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <cstdint>

namespace Hazel {

	class Archive
	{
	public:
		enum class Mode { Read, Write };

		virtual ~Archive() = default;
		virtual Mode GetMode() const = 0;

		// Named structure nodes
		virtual void BeginObject(std::string_view name) = 0;
		virtual void EndObject() = 0;
		virtual void BeginArray(std::string_view name) = 0;
		virtual void EndArray() = 0;

		// Key-existence check (valid only in Read mode inside an object)
		virtual bool HasKey(std::string_view name) = 0;

		// Scalar value overloads
		virtual void Value(std::string_view name, int32_t& v) = 0;
		virtual void Value(std::string_view name, uint32_t& v) = 0;
		virtual void Value(std::string_view name, int64_t& v) = 0;
		virtual void Value(std::string_view name, uint64_t& v) = 0;
		virtual void Value(std::string_view name, float& v) = 0;
		virtual void Value(std::string_view name, double& v) = 0;
		virtual void Value(std::string_view name, bool& v) = 0;
		virtual void Value(std::string_view name, std::string& v) = 0;

		// glm vector types
		virtual void Value(std::string_view name, glm::vec2& v) = 0;
		virtual void Value(std::string_view name, glm::vec3& v) = 0;
		virtual void Value(std::string_view name, glm::vec4& v) = 0;

		// UUID (stored as uint64_t)
		virtual void Value(std::string_view name, UUID& v) = 0;
	};

}
