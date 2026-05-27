#pragma once

#include "Hazel/Serialization/Archive.h"
#include "Hazel/Scene/ComponentReflection.h"
#include "Hazel/Core/Log.h"

#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <type_traits>
#include <typeinfo>

namespace Hazel {

	// ====================================================================
	// Serializer<T> — primary template (Branch 2 + Branch 3)
	// ====================================================================

	template<typename T>
	struct Serializer
	{
		static void Do(Archive& ar, T& val)
		{
			auto type = entt::resolve<T>();
			if (type)
			{
				SerializeReflected(ar, type, val);
			}
			else
			{
				HZ_CORE_WARN("Serialize: unhandled type '{}' (no reflection, no specialization)", typeid(T).name());
			}
		}
	};

	// ====================================================================
	// Branch-2: Reflection-based traversal
	// ====================================================================

	namespace detail {

		// ---- meta_any → compile-type dispatch (write) ----
		using WriteDispatchTypes = std::tuple<int32_t, uint32_t, int64_t, uint64_t, float, double, bool, std::string>;

		template<typename T>
		inline bool TryWriteOne(Archive& ar, std::string_view name, entt::meta_any& value)
		{
			if (auto* p = value.try_cast<T>())
			{
				ar.Value(name, *p);
				return true;
			}
			return false;
		}

		template<typename... Ts>
		inline void WriteMetaValue(Archive& ar, std::string_view name, entt::meta_any value, std::tuple<Ts...>)
		{
			bool ok = (TryWriteOne<Ts>(ar, name, value) || ...);
			if (!ok)
			{
				// Try glm types (handled separately since they need flow sequences)
				if (auto* p = value.try_cast<glm::vec2>()) { ar.Value(name, *p); return; }
				if (auto* p = value.try_cast<glm::vec3>()) { ar.Value(name, *p); return; }
				if (auto* p = value.try_cast<glm::vec4>()) { ar.Value(name, *p); return; }
				HZ_CORE_WARN("Serialize: unhandled field type for '{}'", name);
			}
		}

		inline void WriteMetaValue(Archive& ar, std::string_view name, entt::meta_any value)
		{
			WriteMetaValue(ar, name, value, WriteDispatchTypes{});
		}

		// ---- meta_any → compile-type dispatch (read) ----
		using ReadDispatchTypes = std::tuple<int32_t, uint32_t, int64_t, uint64_t, float, double, bool, std::string>;

		template<typename T>
		inline bool TryReadOne(Archive& ar, std::string_view name, entt::meta_any& value)
		{
			if (auto* p = value.try_cast<T>())
			{
				ar.Value(name, *p);
				return true;
			}
			return false;
		}

		template<typename... Ts>
		inline bool ReadMetaValue(Archive& ar, std::string_view name, entt::meta_any& value, std::tuple<Ts...>)
		{
			bool ok = (TryReadOne<Ts>(ar, name, value) || ...);
			if (!ok)
			{
				// Try glm types
				{
					auto* p = value.try_cast<glm::vec2>();
					if (p) { ar.Value(name, *p); return true; }
				}
				{
					auto* p = value.try_cast<glm::vec3>();
					if (p) { ar.Value(name, *p); return true; }
				}
				{
					auto* p = value.try_cast<glm::vec4>();
					if (p) { ar.Value(name, *p); return true; }
				}
				HZ_CORE_WARN("Serialize: unhandled field type for '{}'", name);
			}
			return ok;
		}

		inline bool ReadMetaValue(Archive& ar, std::string_view name, entt::meta_any& value)
		{
			return ReadMetaValue(ar, name, value, ReadDispatchTypes{});
		}

	} // namespace detail

	template<typename T>
	void SerializeReflected(Archive& ar, entt::meta_type type, T& component)
	{
		if (ar.GetMode() == Archive::Mode::Write)
		{
			for (auto&& [id, data] : type.data())
			{
				if (FieldMeta* fieldMeta = static_cast<FieldMeta*>(data.custom()); fieldMeta && fieldMeta->noSerialize)
					continue;

				auto value = data.get(component);
				detail::WriteMetaValue(ar, data.name(), value);
			}
		}
		else
		{
			for (auto&& [id, data] : type.data())
			{
				if (FieldMeta* fieldMeta = static_cast<FieldMeta*>(data.custom()); fieldMeta && fieldMeta->noSerialize)
					continue;

				if (ar.HasKey(data.name()))
				{
					auto value = data.get(component);
					detail::ReadMetaValue(ar, data.name(), value);
					data.set(component, value);
				}
			}
		}
	}

	// ====================================================================
	// Branch-1: Specializations for scalar types
	// ====================================================================

	#define HZ_SERIALIZE_SCALAR(T) \
		template<> struct Serializer<T> { \
			static void Do(Archive& ar, T& val) { SerializeScalar(ar, val); } \
		};

	template<typename T>
	void SerializeScalar(Archive& ar, T& val)
	{
		// Scalar at root level — pass through (caller provides name via BeginObject/Value)
		// For direct reads/writes, the Archive::Value is used by the reflection loop
		// for named fields. Standalone scalar serialization is a no-op — the value
		// is always contained within a named field.
	}

	// Register scalar types so the primary template doesn't try to reflect them
	HZ_SERIALIZE_SCALAR(int32_t)
	HZ_SERIALIZE_SCALAR(uint32_t)
	HZ_SERIALIZE_SCALAR(int64_t)
	HZ_SERIALIZE_SCALAR(uint64_t)
	HZ_SERIALIZE_SCALAR(float)
	HZ_SERIALIZE_SCALAR(double)
	HZ_SERIALIZE_SCALAR(bool)

	template<>
	struct Serializer<std::string>
	{
		static void Do(Archive& ar, std::string& val) {} // scalar, handled by Archive::Value
	};

	// glm types
	template<> struct Serializer<glm::vec2> { static void Do(Archive& ar, glm::vec2& v) {} };
	template<> struct Serializer<glm::vec3> { static void Do(Archive& ar, glm::vec3& v) {} };
	template<> struct Serializer<glm::vec4> { static void Do(Archive& ar, glm::vec4& v) {} };

	// ====================================================================
	// Unified entry point
	// ====================================================================

	template<typename T>
	void Serialize(Archive& ar, T& val)
	{
		Serializer<T>::Do(ar, val);
	}

}
