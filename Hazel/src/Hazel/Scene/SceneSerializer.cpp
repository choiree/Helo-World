#include "hzpch.h"
#include "SceneSerializer.h"

#include "Entity.h"
#include "Components.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Core/UUID.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Scene/SpriteSheetSerializer.h"
#include "Hazel/Renderer/SpriteSheet.h"
#include "Hazel/Project/AssetManager.h"

#include <fstream>

#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Hazel::UUID>
	{
		static Node encode(const Hazel::UUID& uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, Hazel::UUID& uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

}

namespace Hazel {

#define WRITE_SCRIPT_FIELD(FieldType, Type)           \
			case ScriptFieldType::FieldType:          \
				out << scriptField.GetValue<Type>();  \
				break

#define READ_SCRIPT_FIELD(FieldType, Type)             \
	case ScriptFieldType::FieldType:                   \
	{                                                  \
		Type data = scriptField["Data"].as<Type>();    \
		fieldInstance.SetValue(data);                  \
		break;                                         \
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
			case Rigidbody2DComponent::BodyType::Static:    return "Static";
			case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
			case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		HZ_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

		HZ_CORE_ASSERT(false, "Unknown body type");
		return Rigidbody2DComponent::BodyType::Static;
	}

	// --- Reflection-driven serialization helpers ---

	static void EmitMetaValue(YAML::Emitter& out, entt::meta_any value)
	{
		auto type = value.type();
		if (type == entt::resolve<glm::vec2>())
			out << value.cast<glm::vec2>();
		else if (type == entt::resolve<glm::vec3>())
			out << value.cast<glm::vec3>();
		else if (type == entt::resolve<glm::vec4>())
			out << value.cast<glm::vec4>();
		else if (type == entt::resolve<float>())
			out << value.cast<float>();
		else if (type == entt::resolve<int>())
			out << value.cast<int>();
		else if (type == entt::resolve<bool>())
			out << value.cast<bool>();
		else if (type == entt::resolve<uint32_t>())
			out << value.cast<uint32_t>();
		else if (type == entt::resolve<std::string>())
			out << value.cast<std::string>();
		else
			out << YAML::Null;
	}

	template<typename T>
	static void ReadMetaValue(const YAML::Node& node, entt::meta_data& data, T& component)
	{
		auto type = data.type();
		if (type == entt::resolve<glm::vec2>())
			data.set(component, node.as<glm::vec2>());
		else if (type == entt::resolve<glm::vec3>())
			data.set(component, node.as<glm::vec3>());
		else if (type == entt::resolve<glm::vec4>())
			data.set(component, node.as<glm::vec4>());
		else if (type == entt::resolve<float>())
			data.set(component, node.as<float>());
		else if (type == entt::resolve<int>())
			data.set(component, node.as<int>());
		else if (type == entt::resolve<bool>())
			data.set(component, node.as<bool>());
		else if (type == entt::resolve<uint32_t>())
			data.set(component, node.as<uint32_t>());
		else if (type == entt::resolve<std::string>())
			data.set(component, node.as<std::string>());
	}

	template<typename T>
	static void SerializeComponent(YAML::Emitter& out, T& component)
	{
		auto type = entt::resolve<T>();
		if (!type) return;

		for (auto&& [id, data] : type.data())
		{
			auto value = data.get(component);
			out << YAML::Key << data.name();
			EmitMetaValue(out, value);
		}
	}

	template<typename T>
	static void DeserializeComponent(const YAML::Node& node, T& component)
	{
		auto type = entt::resolve<T>();
		if (!type) return;

		for (auto&& [id, data] : type.data())
		{
			auto fieldNode = node[data.name()];
			if (fieldNode)
				ReadMetaValue(fieldNode, data, component);
		}
	}

	// ---------------------------------------------------

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		HZ_CORE_ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			SerializeComponent(out, tc);

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<ScriptComponent>())
		{
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();

			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent
			out << YAML::Key << "ClassName" << YAML::Value << scriptComponent.ClassName;

			// Fields
			Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(scriptComponent.ClassName);
			const auto& fields = entityClass->GetFields();
			if (fields.size() > 0)
			{
				out << YAML::Key << "ScriptFields" << YAML::Value;
				auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					if (entityFields.find(name) == entityFields.end())
						continue;

					out << YAML::BeginMap; // ScriptField
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.Type);

					out << YAML::Key << "Data" << YAML::Value;
					ScriptFieldInstance& scriptField = entityFields.at(name);

					switch (field.Type)
					{
						WRITE_SCRIPT_FIELD(Float,   float     );
						WRITE_SCRIPT_FIELD(Double,  double    );
						WRITE_SCRIPT_FIELD(Bool,    bool      );
						WRITE_SCRIPT_FIELD(Char,    char      );
						WRITE_SCRIPT_FIELD(Byte,    int8_t    );
						WRITE_SCRIPT_FIELD(Short,   int16_t   );
						WRITE_SCRIPT_FIELD(Int,     int32_t   );
						WRITE_SCRIPT_FIELD(Long,    int64_t   );
						WRITE_SCRIPT_FIELD(UByte,   uint8_t   );
						WRITE_SCRIPT_FIELD(UShort,  uint16_t  );
						WRITE_SCRIPT_FIELD(UInt,    uint32_t  );
						WRITE_SCRIPT_FIELD(ULong,   uint64_t  );
						WRITE_SCRIPT_FIELD(Vector2, glm::vec2 );
						WRITE_SCRIPT_FIELD(Vector3, glm::vec3 );
						WRITE_SCRIPT_FIELD(Vector4, glm::vec4 );
						WRITE_SCRIPT_FIELD(Entity,  UUID      );
					}
					out << YAML::EndMap; // ScriptFields
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // ScriptComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			auto& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << spriteRendererComponent.Color;
			if (spriteRendererComponent.Texture)
				out << YAML::Key << "TexturePath" << YAML::Value << spriteRendererComponent.Texture->GetPath();

			if (spriteRendererComponent.SubTexture)
			{
				out << YAML::Key << "SubTexture" << YAML::Value;
				out << YAML::BeginMap; // SubTexture
				out << YAML::Key << "UV0" << YAML::Value << spriteRendererComponent.SubTexture->GetUV0();
				out << YAML::Key << "UV1" << YAML::Value << spriteRendererComponent.SubTexture->GetUV1();
				out << YAML::EndMap; // SubTexture
			}

			out << YAML::Key << "TilingFactor" << YAML::Value << spriteRendererComponent.TilingFactor;

			out << YAML::EndMap; // SpriteRendererComponent
		}

		if (entity.HasComponent<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent";
			out << YAML::BeginMap;
			SerializeComponent(out, entity.GetComponent<CircleRendererComponent>());
			out << YAML::EndMap;
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap; // Rigidbody2DComponent

			auto& rb2dComponent = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2dComponent.Type);
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2dComponent.FixedRotation;

			out << YAML::EndMap; // Rigidbody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap;
			SerializeComponent(out, entity.GetComponent<BoxCollider2DComponent>());
			out << YAML::EndMap;
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap;
			SerializeComponent(out, entity.GetComponent<CircleCollider2DComponent>());
			out << YAML::EndMap;
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap;
			SerializeComponent(out, entity.GetComponent<TextComponent>());
			out << YAML::EndMap;
		}

		if (entity.HasComponent<SpriteAnimationComponent>())
		{
			out << YAML::Key << "SpriteAnimationComponent";
			out << YAML::BeginMap; // SpriteAnimationComponent

			auto& anim = entity.GetComponent<SpriteAnimationComponent>();
			out << YAML::Key << "SpeedMultiplier" << YAML::Value << anim.SpeedMultiplier;
			out << YAML::Key << "Playing" << YAML::Value << anim.Playing;
			out << YAML::Key << "CurrentFrame" << YAML::Value << anim.CurrentFrame;
			out << YAML::Key << "FrameTimer" << YAML::Value << anim.FrameTimer;
			if (anim.CurrentClip)
				out << YAML::Key << "CurrentClip" << YAML::Value << anim.CurrentClip->GetName();

			if (!anim.Clips.empty())
			{
				out << YAML::Key << "Clips" << YAML::Value << YAML::BeginSeq;
				for (const auto& [name, clip] : anim.Clips)
				{
					if (clip->GetFrameCount() == 0)
						continue;

					const auto& firstFrame = clip->GetFrame(0);
					if (!firstFrame.SubTexture || !firstFrame.SubTexture->GetTexture())
						continue;

					std::string texturePath = firstFrame.SubTexture->GetTexture()->GetPath();
					if (texturePath.empty())
						continue;

					out << YAML::BeginMap; // Clip
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Loop" << YAML::Value << clip->IsLooping();
					out << YAML::Key << "TexturePath" << YAML::Value << texturePath;

					if (!clip->GetSpriteSheetPath().empty())
						out << YAML::Key << "SpriteSheet" << YAML::Value << clip->GetSpriteSheetPath();

					out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
					for (size_t i = 0; i < clip->GetFrameCount(); i++)
					{
						const auto& frame = clip->GetFrame(i);
						out << YAML::BeginMap; // Frame
						out << YAML::Key << "Duration" << YAML::Value << frame.Duration;
						if (frame.CellX >= 0 && frame.CellY >= 0)
						{
							out << YAML::Key << "Cell" << YAML::Value;
							out << YAML::Flow << YAML::BeginSeq << frame.CellX << frame.CellY << YAML::EndSeq;
						}
						else
						{
							out << YAML::Key << "UV0" << YAML::Value << frame.SubTexture->GetUV0();
							out << YAML::Key << "UV1" << YAML::Value << frame.SubTexture->GetUV1();
						}
						out << YAML::EndMap; // Frame
					}
					out << YAML::EndSeq; // Frames

					out << YAML::EndMap; // Clip
				}
				out << YAML::EndSeq; // Clips
			}

			out << YAML::EndMap; // SpriteAnimationComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		auto view = m_Scene->m_Registry.view<entt::entity>();
		view.each([&](entt::entity entityID) {
			Entity entity = { entityID, m_Scene.get() };
			if (!entity)
				return;
			SerializeEntity(out, entity);
			});
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
	}


	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		AssetManager::BeginScene();
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(filepath);
		}
		catch (YAML::ParserException e)
		{
			HZ_CORE_ERROR("Failed to load .hazel file '{0}'\n     {1}", filepath, e.what());
			AssetManager::EndScene();
			return false;
		}


		if (!data["Scene"])
		{
			AssetManager::EndScene();
			return false;
		}

		std::string sceneName = data["Scene"].as<std::string>();
		HZ_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				HZ_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					DeserializeComponent(transformComponent, tc);
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto scriptComponent = entity["ScriptComponent"];
				if (scriptComponent)
				{
					auto& sc = deserializedEntity.AddComponent<ScriptComponent>();
					sc.ClassName = scriptComponent["ClassName"].as<std::string>();

					auto scriptFields = scriptComponent["ScriptFields"];
					if (scriptFields)
					{
						Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
						if (entityClass)
						{
							const auto& fields = entityClass->GetFields();
							auto& entityFields = ScriptEngine::GetScriptFieldMap(deserializedEntity);

							for (auto scriptField : scriptFields)
							{
								std::string name = scriptField["Name"].as<std::string>();
								std::string typeString = scriptField["Type"].as<std::string>();
								ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

								ScriptFieldInstance& fieldInstance = entityFields[name];

								// TODO(Yan): turn this assert into Hazelnut log warning
								HZ_CORE_ASSERT(fields.find(name) != fields.end());

								if (fields.find(name) == fields.end())
									continue;

								fieldInstance.Field = fields.at(name);

								switch (type)
								{
									READ_SCRIPT_FIELD(Float, float);
									READ_SCRIPT_FIELD(Double, double);
									READ_SCRIPT_FIELD(Bool, bool);
									READ_SCRIPT_FIELD(Char, char);
									READ_SCRIPT_FIELD(Byte, int8_t);
									READ_SCRIPT_FIELD(Short, int16_t);
									READ_SCRIPT_FIELD(Int, int32_t);
									READ_SCRIPT_FIELD(Long, int64_t);
									READ_SCRIPT_FIELD(UByte, uint8_t);
									READ_SCRIPT_FIELD(UShort, uint16_t);
									READ_SCRIPT_FIELD(UInt, uint32_t);
									READ_SCRIPT_FIELD(ULong, uint64_t);
									READ_SCRIPT_FIELD(Vector2, glm::vec2);
									READ_SCRIPT_FIELD(Vector3, glm::vec3);
									READ_SCRIPT_FIELD(Vector4, glm::vec4);
									READ_SCRIPT_FIELD(Entity, UUID);
								}
							}
						}
					}

				}

				auto spriteRendererComponent = entity["SpriteRendererComponent"];
				if (spriteRendererComponent)
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();
					src.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					if (spriteRendererComponent["TexturePath"])
					{
						std::string texturePath = spriteRendererComponent["TexturePath"].as<std::string>();
						src.Texture = AssetManager::Load<Texture2D>(texturePath);
					}

					if (spriteRendererComponent["TilingFactor"])
						src.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>();

					auto subTextureNode = spriteRendererComponent["SubTexture"];
					if (subTextureNode && src.Texture)
					{
						glm::vec2 uv0 = subTextureNode["UV0"].as<glm::vec2>();
						glm::vec2 uv1 = subTextureNode["UV1"].as<glm::vec2>();
						src.SubTexture = SubTexture2D::Create(src.Texture, uv0, uv1);
					}
				}

				auto circleRendererComponent = entity["CircleRendererComponent"];
				if (circleRendererComponent)
				{
					auto& crc = deserializedEntity.AddComponent<CircleRendererComponent>();
					DeserializeComponent(circleRendererComponent, crc);
				}

				auto rigidbody2DComponent = entity["Rigidbody2DComponent"];
				if (rigidbody2DComponent)
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				auto boxCollider2DComponent = entity["BoxCollider2DComponent"];
				if (boxCollider2DComponent)
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					DeserializeComponent(boxCollider2DComponent, bc2d);
				}

				auto circleCollider2DComponent = entity["CircleCollider2DComponent"];
				if (circleCollider2DComponent)
				{
					auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					DeserializeComponent(circleCollider2DComponent, cc2d);
				}

				auto textComponent = entity["TextComponent"];
				if (textComponent)
				{
					auto& tc = deserializedEntity.AddComponent<TextComponent>();
					DeserializeComponent(textComponent, tc);
				}

				auto animComponent = entity["SpriteAnimationComponent"];
				if (animComponent)
				{
					auto& anim = deserializedEntity.AddComponent<SpriteAnimationComponent>();
					if (animComponent["SpeedMultiplier"])
						anim.SpeedMultiplier = animComponent["SpeedMultiplier"].as<float>();
					if (animComponent["Playing"])
						anim.Playing = animComponent["Playing"].as<bool>();
					if (animComponent["CurrentFrame"])
						anim.CurrentFrame = animComponent["CurrentFrame"].as<int>();
					if (animComponent["FrameTimer"])
						anim.FrameTimer = animComponent["FrameTimer"].as<float>();

					// Reconstruct clips from serialized data
					auto clipsNode = animComponent["Clips"];
					if (clipsNode)
					{
						for (auto clipNode : clipsNode)
						{
							std::string clipName = clipNode["Name"].as<std::string>();
							bool loop = clipNode["Loop"] ? clipNode["Loop"].as<bool>() : true;
							std::string texturePath = clipNode["TexturePath"] ? clipNode["TexturePath"].as<std::string>() : "";

							// Load SpriteSheet if present
							std::string sheetPath = clipNode["SpriteSheet"] ? clipNode["SpriteSheet"].as<std::string>() : "";
							Ref<SpriteSheet> spriteSheet;
							Ref<Texture2D> texture;
							if (!sheetPath.empty())
							{
								spriteSheet = AssetManager::Load<SpriteSheet>(sheetPath);
								if (spriteSheet)
									texture = spriteSheet->GetTexture();
							}

							// Fall back to direct texture path
							if (!texture && !texturePath.empty())
							{
								texture = AssetManager::Load<Texture2D>(texturePath);
							}

							if (!texture || !texture->IsLoaded())
								continue;

							auto clip = AnimationClip::Create(clipName, loop);
							if (spriteSheet)
								clip->SetSpriteSheetPath(sheetPath);

							auto framesNode = clipNode["Frames"];
							if (framesNode)
							{
								for (auto frameNode : framesNode)
								{
									float duration = frameNode["Duration"] ? frameNode["Duration"].as<float>() : 0.1f;
									int cellX = -1, cellY = -1;
									Ref<SubTexture2D> subTex;

									if (frameNode["Cell"] && spriteSheet)
									{
										auto cellNode = frameNode["Cell"];
										cellX = cellNode[0].as<int>();
										cellY = cellNode[1].as<int>();
										glm::vec2 uv0 = spriteSheet->CellToUV0(cellX, cellY);
										glm::vec2 uv1 = spriteSheet->CellToUV1(cellX, cellY);
										subTex = SubTexture2D::Create(texture, uv0, uv1);
									}
									else
									{
										glm::vec2 uv0 = frameNode["UV0"].as<glm::vec2>();
										glm::vec2 uv1 = frameNode["UV1"].as<glm::vec2>();
										subTex = SubTexture2D::Create(texture, uv0, uv1);
									}

									clip->AddFrame(subTex, duration);
									if (cellX >= 0)
									{
										auto& frames = const_cast<std::vector<AnimationFrame>&>(clip->GetFrames());
										frames.back().CellX = cellX;
										frames.back().CellY = cellY;
									}
								}
							}

							anim.Clips[clipName] = clip;
						}
					}

					// Restore current clip
					if (animComponent["CurrentClip"])
					{
						std::string currentClipName = animComponent["CurrentClip"].as<std::string>();
						auto it = anim.Clips.find(currentClipName);
						if (it != anim.Clips.end())
							anim.CurrentClip = it->second;
					}
				}
			}
		}


		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		// Not implemented
		HZ_CORE_ASSERT(false);
		return false;
	}

}
