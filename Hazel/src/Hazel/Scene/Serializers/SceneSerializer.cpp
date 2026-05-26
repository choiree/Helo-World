#include "hzpch.h"
#include "SceneSerializer.h"

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Core/UUID.h"

#include "Hazel/Project/Project.h"
#include "Hazel/Scene/Serializers/SpriteSheetSerializer.h"
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

	// ==================================================================
	//  Type-level YAML helpers
	// ==================================================================

	using YamlPrimitiveTypes = std::tuple<float, int, bool, uint32_t, std::string>;
	using YamlCustomTypes    = std::tuple<glm::vec2, glm::vec3, glm::vec4>;

	template<typename... Ts>
	static bool TryEmitAs(YAML::Emitter& out, entt::meta_any& value, std::tuple<Ts...>)
	{
		return ((value.type() == entt::resolve<Ts>() && (out << value.cast<Ts>(), true)) || ...);
	}

	static void EmitMetaValue(YAML::Emitter& out, entt::meta_any value)
	{
		if (TryEmitAs(out, value, YamlCustomTypes{}))
			return;
		if (TryEmitAs(out, value, YamlPrimitiveTypes{}))
			return;
		out << YAML::Null;
	}

	template<typename T, typename... Ts>
	static bool TryReadAs(const YAML::Node& node, entt::meta_data& data, T& component, std::tuple<Ts...>)
	{
		return ((data.type() == entt::resolve<Ts>() && (data.set(component, node.as<Ts>()), true)) || ...);
	}

	template<typename T>
	static void ReadMetaValue(const YAML::Node& node, entt::meta_data& data, T& component)
	{
		if (TryReadAs(node, data, component, YamlCustomTypes{}))
			return;
		if (TryReadAs(node, data, component, YamlPrimitiveTypes{}))
			return;
	}

	// ==================================================================
	//  Component-level reflection helpers
	// ==================================================================

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

	// ==================================================================
	//  ComponentSerializer — default (reflection) + custom specializations
	// ==================================================================

	template<typename T>
	struct ComponentSerializer
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<T>())
				return;
			auto type = entt::resolve<T>();
			if (!type)
				return;
			out << YAML::Key << type.name();
			out << YAML::BeginMap;
			SerializeComponent(out, entity.GetComponent<T>());
			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<CameraComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<CameraComponent>())
				return;
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap;

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap;

			SerializeComponent(out, cameraComponent);

			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<ScriptComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<ScriptComponent>())
				return;
			auto& scriptComponent = entity.GetComponent<ScriptComponent>();

			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap;
			SerializeComponent(out, scriptComponent);

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

					out << YAML::BeginMap;
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
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<SpriteRendererComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<SpriteRendererComponent>())
				return;
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap;

			auto& src = entity.GetComponent<SpriteRendererComponent>();
			SerializeComponent(out, src);
			if (src.Texture)
				out << YAML::Key << "TexturePath" << YAML::Value << src.Texture->GetPath();

			if (src.SubTexture)
			{
				out << YAML::Key << "SubTexture" << YAML::Value;
				out << YAML::BeginMap;
				out << YAML::Key << "UV0" << YAML::Value << src.SubTexture->GetUV0();
				out << YAML::Key << "UV1" << YAML::Value << src.SubTexture->GetUV1();
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<Rigidbody2DComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<Rigidbody2DComponent>())
				return;
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap;

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2d.Type);
			SerializeComponent(out, rb2d);

			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<SpriteAnimationComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<SpriteAnimationComponent>())
				return;
			out << YAML::Key << "SpriteAnimationComponent";
			out << YAML::BeginMap;

			auto& anim = entity.GetComponent<SpriteAnimationComponent>();
			SerializeComponent(out, anim);
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

					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Loop" << YAML::Value << clip->IsLooping();
					out << YAML::Key << "TexturePath" << YAML::Value << texturePath;

					if (!clip->GetSpriteSheetPath().empty())
						out << YAML::Key << "SpriteSheet" << YAML::Value << clip->GetSpriteSheetPath();

					out << YAML::Key << "Frames" << YAML::Value << YAML::BeginSeq;
					for (size_t i = 0; i < clip->GetFrameCount(); i++)
					{
						const auto& frame = clip->GetFrame(i);
						out << YAML::BeginMap;
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
						out << YAML::EndMap;
					}
					out << YAML::EndSeq;

					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}
	};

	template<>
	struct ComponentSerializer<TileMapComponent>
	{
		static void Serialize(YAML::Emitter& out, Entity entity)
		{
			if (!entity.HasComponent<TileMapComponent>())
				return;
			out << YAML::Key << "TileMapComponent";
			out << YAML::BeginMap;

			auto& tmc = entity.GetComponent<TileMapComponent>();
			if (tmc.Map)
				out << YAML::Key << "MapPath" << YAML::Value << tmc.Map->GetSourcePath();
			SerializeComponent(out, tmc);

			out << YAML::EndMap;
		}
	};

	// ==================================================================
	//  ComponentDeserializer — default (reflection) + custom specializations
	// ==================================================================

	template<typename T>
	struct ComponentDeserializer
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto type = entt::resolve<T>();
			if (!type)
				return;
			auto node = entityNode[type.name()];
			if (!node)
				return;
			auto& comp = entity.AddOrReplaceComponent<T>();
			DeserializeComponent(node, comp);
		}
	};

	template<>
	struct ComponentDeserializer<CameraComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["CameraComponent"];
			if (!node)
				return;
			auto& cc = entity.AddOrReplaceComponent<CameraComponent>();

			auto cameraProps = node["Camera"];
			if (cameraProps)
			{
				cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());
				cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
				cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
				cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());
				cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
				cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
				cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());
			}

			DeserializeComponent(node, cc);
		}
	};

	template<>
	struct ComponentDeserializer<ScriptComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["ScriptComponent"];
			if (!node)
				return;
			auto& sc = entity.AddOrReplaceComponent<ScriptComponent>();
			DeserializeComponent(node, sc);

			auto scriptFields = node["ScriptFields"];
			if (scriptFields)
			{
				Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
				if (entityClass)
				{
					const auto& fields = entityClass->GetFields();
					auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

					for (auto scriptField : scriptFields)
					{
						std::string name = scriptField["Name"].as<std::string>();
						std::string typeString = scriptField["Type"].as<std::string>();
						ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

						ScriptFieldInstance& fieldInstance = entityFields[name];

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
	};

	template<>
	struct ComponentDeserializer<SpriteRendererComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["SpriteRendererComponent"];
			if (!node)
				return;
			auto& src = entity.AddOrReplaceComponent<SpriteRendererComponent>();

			if (node["TexturePath"])
			{
				std::string texturePath = node["TexturePath"].as<std::string>();
				src.Texture = AssetManager::Load<Texture2D>(texturePath);
			}

			auto subTextureNode = node["SubTexture"];
			if (subTextureNode && src.Texture)
			{
				glm::vec2 uv0 = subTextureNode["UV0"].as<glm::vec2>();
				glm::vec2 uv1 = subTextureNode["UV1"].as<glm::vec2>();
				src.SubTexture = SubTexture2D::Create(src.Texture, uv0, uv1);
			}

			DeserializeComponent(node, src);
		}
	};

	template<>
	struct ComponentDeserializer<Rigidbody2DComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["Rigidbody2DComponent"];
			if (!node)
				return;
			auto& rb2d = entity.AddOrReplaceComponent<Rigidbody2DComponent>();

			if (node["BodyType"])
				rb2d.Type = RigidBody2DBodyTypeFromString(node["BodyType"].as<std::string>());

			DeserializeComponent(node, rb2d);
		}
	};

	template<>
	struct ComponentDeserializer<SpriteAnimationComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["SpriteAnimationComponent"];
			if (!node)
				return;
			auto& anim = entity.AddOrReplaceComponent<SpriteAnimationComponent>();
			DeserializeComponent(node, anim);

			// Reconstruct clips from serialized data
			auto clipsNode = node["Clips"];
			if (clipsNode)
			{
				for (auto clipNode : clipsNode)
				{
					std::string clipName = clipNode["Name"].as<std::string>();
					bool loop = clipNode["Loop"] ? clipNode["Loop"].as<bool>() : true;
					std::string texturePath = clipNode["TexturePath"] ? clipNode["TexturePath"].as<std::string>() : "";

					std::string sheetPath = clipNode["SpriteSheet"] ? clipNode["SpriteSheet"].as<std::string>() : "";
					Ref<SpriteSheet> spriteSheet;
					Ref<Texture2D> texture;
					if (!sheetPath.empty())
					{
						spriteSheet = AssetManager::Load<SpriteSheet>(sheetPath);
						if (spriteSheet)
							texture = spriteSheet->GetTexture();
					}

					if (!texture && !texturePath.empty())
						texture = AssetManager::Load<Texture2D>(texturePath);

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
			if (node["CurrentClip"])
			{
				std::string currentClipName = node["CurrentClip"].as<std::string>();
				auto it = anim.Clips.find(currentClipName);
				if (it != anim.Clips.end())
					anim.CurrentClip = it->second;
			}
		}
	};

	template<>
	struct ComponentDeserializer<TileMapComponent>
	{
		static void Deserialize(const YAML::Node& entityNode, Entity entity)
		{
			auto node = entityNode["TileMapComponent"];
			if (!node)
				return;
			auto& tmc = entity.AddOrReplaceComponent<TileMapComponent>();

			if (node["MapPath"])
				tmc.Map = AssetManager::Load<TileMapAsset>(node["MapPath"].as<std::string>());

			DeserializeComponent(node, tmc);
		}
	};

	// ==================================================================
	//  Type list + fold expressions
	// ==================================================================

	using SceneComponentTypes = std::tuple<
		TagComponent,
		TransformComponent,
		CameraComponent,
		ScriptComponent,
		SpriteRendererComponent,
		CircleRendererComponent,
		Rigidbody2DComponent,
		BoxCollider2DComponent,
		CircleCollider2DComponent,
		TextComponent,
		SpriteAnimationComponent,
		TileMapComponent
	>;

	template<typename... Ts>
	static void SerializeAllComponents(YAML::Emitter& out, Entity entity, std::tuple<Ts...>)
	{
		(ComponentSerializer<Ts>::Serialize(out, entity), ...);
	}

	template<typename... Ts>
	static void DeserializeAllComponents(const YAML::Node& entityNode, Entity entity, std::tuple<Ts...>)
	{
		(ComponentDeserializer<Ts>::Deserialize(entityNode, entity), ...);
	}

	// ==================================================================
	//  Entity-level
	// ==================================================================

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		HZ_CORE_ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		SerializeAllComponents(out, entity, SceneComponentTypes{});

		out << YAML::EndMap;
	}

	// ==================================================================
	//  SceneSerializer
	// ==================================================================

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
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
				auto tagNode = entity["TagComponent"];
				if (tagNode)
					name = tagNode["Tag"].as<std::string>();

				HZ_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				DeserializeAllComponents(entity, deserializedEntity, SceneComponentTypes{});
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
