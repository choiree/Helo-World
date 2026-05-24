#pragma once

#include "Hazel/Core/Base.h"

#include <entt/entt.hpp>

namespace Hazel {

	// Traits to tag component behavior (stored in entt meta_type::traits, 16-bit max)
	enum class ComponentTrait : uint16_t
	{
		None = 0,
		Serializable = 1 << 0,   // Serialize to scene file
		HiddenInUI = 1 << 1,     // Don't show in editor property panel
		EditorOnly = 1 << 2,     // Strip from runtime builds
	};

	constexpr ComponentTrait operator|(ComponentTrait a, ComponentTrait b)
	{
		return static_cast<ComponentTrait>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
	}

	constexpr bool operator&(ComponentTrait a, ComponentTrait b)
	{
		return (static_cast<uint16_t>(a) & static_cast<uint16_t>(b)) != 0;
	}

	// Per-field metadata (stored via entt meta_data::custom<FieldMeta>).
	// Only ONE custom value per meta object, so all field metadata lives in one struct.
	struct FieldMeta
	{
		float speed = 0.1f;       // DragFloat/DragInt speed
		float min = 0.0f;         // DragFloat min (min==max means no clamp)
		float max = 0.0f;         // DragFloat max
		float resetValue = 0.0f;  // DrawVec3Control reset button value
		const char* tooltip = nullptr; // hover tooltip text
	};

	// Convenience macros for attaching FieldMeta to a data member.
	// Usage: factory.data<&T::Field>("Name") HZ_FIELD_SPEED(0.25f) HZ_FIELD_RANGE(0, 100);
	// NOTE: Multiple macros DON'T compose — they overwrite. Use designated initializers
	// for multiple attrs: .custom<FieldMeta>({.speed = 0.25f, .min = 0.0f, .max = 100.0f})
	#define HZ_FIELD_SPEED(v)        .custom<FieldMeta>({.speed = v})
	#define HZ_FIELD_RANGE(lo, hi)   .custom<FieldMeta>({.min = lo, .max = hi})
	#define HZ_FIELD_RESET(v)        .custom<FieldMeta>({.resetValue = v})
	#define HZ_FIELD_TOOLTIP(s)      .custom<FieldMeta>({.tooltip = s})

	// Register a component type with the entt meta system.
	// Usage:
	//   HZ_REGISTER_COMPONENT(TransformComponent,
	//       factory.traits(ComponentTrait::Serializable);
	//       factory.data<&TransformComponent::Translation>("Translation"_hs);
	//       factory.data<&TransformComponent::Rotation>("Rotation"_hs);
	//       factory.data<&TransformComponent::Scale>("Scale"_hs);
	//   );
	#define HZ_REGISTER_COMPONENT(T, ...) \
		static int _hz_reg_##T = []() -> int { \
			entt::meta_factory<T> factory{}; \
			factory.type(#T); \
			factory.traits(ComponentTrait::Serializable); \
			__VA_ARGS__; \
			return 0; \
		}()

}
