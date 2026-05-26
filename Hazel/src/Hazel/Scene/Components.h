#pragma once

#include "SceneCamera.h"
#include "ComponentReflection.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/Renderer/SubTexture2D.h"
#include "Hazel/Renderer/AnimationClip.h"
#include "Hazel/Renderer/Font.h"
#include "Hazel/Renderer/TileMapAsset.h"
#include "Hazel/Renderer/TileSetAsset.h"
#include "Hazel/Renderer/PaletteAsset.h"

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "box2d/box2d.h"

namespace Hazel {

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(UUID id) : ID(id) {}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	HZ_REGISTER_COMPONENT(TagComponent,
		factory.data<&TagComponent::Tag>("Tag");
	);

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	HZ_REGISTER_COMPONENT(TransformComponent,
		factory.data<&TransformComponent::Translation>("Translation");
		factory.data<&TransformComponent::Rotation>("Rotation");
		factory.data<&TransformComponent::Scale>("Scale")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .resetValue = 1.0f});
	);

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		Ref<SubTexture2D> SubTexture;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}
	};

	HZ_REGISTER_COMPONENT(SpriteRendererComponent,
		factory.data<&SpriteRendererComponent::Color>("Color");
		factory.data<&SpriteRendererComponent::TilingFactor>("TilingFactor");
	);

	struct CircleRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.005f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(CircleRendererComponent,
		factory.data<&CircleRendererComponent::Color>("Color");
		factory.data<&CircleRendererComponent::Thickness>("Thickness")
			.custom<FieldMeta>(FieldMeta{.speed = 0.05f});
		factory.data<&CircleRendererComponent::Fade>("Fade")
			.custom<FieldMeta>(FieldMeta{.speed = 0.001f});
	);

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(CameraComponent,
		factory.data<&CameraComponent::Primary>("Primary");
		factory.data<&CameraComponent::FixedAspectRatio>("FixedAspectRatio");
	);

	struct ScriptComponent
	{
		std::string ClassName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(ScriptComponent,
		factory.data<&ScriptComponent::ClassName>("ClassName");
	);

	// Forward declaration
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	struct SpriteAnimationComponent
	{
		std::unordered_map<std::string, Ref<AnimationClip>> Clips;

		Ref<AnimationClip> CurrentClip;
		int CurrentFrame = 0;
		float FrameTimer = 0.0f;
		bool Playing = true;
		float SpeedMultiplier = 1.0f;

		bool Finished = false;
		std::function<void()> OnFinished;

		SpriteAnimationComponent() = default;
		SpriteAnimationComponent(const SpriteAnimationComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(SpriteAnimationComponent,
		factory.data<&SpriteAnimationComponent::SpeedMultiplier>("SpeedMultiplier")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .min = 0.0f});
		factory.data<&SpriteAnimationComponent::Playing>("Playing");
		factory.data<&SpriteAnimationComponent::CurrentFrame>("CurrentFrame");
		factory.data<&SpriteAnimationComponent::FrameTimer>("FrameTimer");
	);

	// Physics

	struct Rigidbody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Storage for runtime
		b2BodyId RuntimeBody = b2_nullBodyId;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(Rigidbody2DComponent,
		factory.data<&Rigidbody2DComponent::FixedRotation>("FixedRotation");
	);

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO(Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;

		// Storage for runtime
		b2ShapeId RuntimeFixture = b2_nullShapeId;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(BoxCollider2DComponent,
		factory.data<&BoxCollider2DComponent::Offset>("Offset");
		factory.data<&BoxCollider2DComponent::Size>("Size")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .min = 0.01f});
		factory.data<&BoxCollider2DComponent::Density>("Density")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .min = 0.0f});
		factory.data<&BoxCollider2DComponent::Friction>("Friction")
			.custom<FieldMeta>(FieldMeta{.speed = 0.05f, .min = 0.0f, .max = 1.0f});
		factory.data<&BoxCollider2DComponent::Restitution>("Restitution")
			.custom<FieldMeta>(FieldMeta{.speed = 0.05f, .min = 0.0f, .max = 1.0f});
	);

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;

		// TODO(Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;

		// Storage for runtime
		b2ShapeId RuntimeFixture = b2_nullShapeId;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(CircleCollider2DComponent,
		factory.data<&CircleCollider2DComponent::Offset>("Offset");
		factory.data<&CircleCollider2DComponent::Radius>("Radius")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .min = 0.01f});
		factory.data<&CircleCollider2DComponent::Density>("Density")
			.custom<FieldMeta>(FieldMeta{.speed = 0.1f, .min = 0.0f});
		factory.data<&CircleCollider2DComponent::Friction>("Friction")
			.custom<FieldMeta>(FieldMeta{.speed = 0.05f, .min = 0.0f, .max = 1.0f});
		factory.data<&CircleCollider2DComponent::Restitution>("Restitution")
			.custom<FieldMeta>(FieldMeta{.speed = 0.05f, .min = 0.0f, .max = 1.0f});
	);

	struct TextComponent
	{
		std::string TextString;

		// TODO 解决依赖问题
		//Ref<Font> FontAsset = Font::GetDefault();
		glm::vec4 Color{ 1.0f };
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
	};

	HZ_REGISTER_COMPONENT(TextComponent,
		factory.data<&TextComponent::TextString>("TextString")
			.custom<FieldMeta>(FieldMeta{.tooltip = "Text string to display"});
		factory.data<&TextComponent::Color>("Color");
		factory.data<&TextComponent::Kerning>("Kerning")
			.custom<FieldMeta>(FieldMeta{.speed = 0.025f});
		factory.data<&TextComponent::LineSpacing>("LineSpacing")
			.custom<FieldMeta>(FieldMeta{.speed = 0.025f});
	);

	struct TileMapComponent
	{
		Ref<TileMapAsset> Map;
		bool Visible = true;

		// Runtime-switchable (not serialized)
		Ref<TileSetAsset> CurrentBottomTileSet;
		Ref<TileSetAsset> CurrentTopTileSet;
		Ref<PaletteAsset> CurrentPalette;

		TileMapComponent() = default;
		TileMapComponent(const TileMapComponent&) = default;
	};

	HZ_REGISTER_COMPONENT(TileMapComponent,
		factory.data<&TileMapComponent::Visible>("Visible");
	);

	// AllComponents is needed by Scene::Copy and DuplicateEntity which
	// require compile-time types for efficient entt::view<T>() iteration.
	// entt has no runtime "iterate all components of an entity" API.
	// All other component boilerplate (OnComponentAdded, serialization) is
	// now driven by HZ_REGISTER_COMPONENT / entt::resolve().
	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<TransformComponent, SpriteRendererComponent,
			CircleRendererComponent, CameraComponent, ScriptComponent,
			NativeScriptComponent, SpriteAnimationComponent,
			Rigidbody2DComponent, BoxCollider2DComponent,
			CircleCollider2DComponent, TextComponent, TileMapComponent>;

}
