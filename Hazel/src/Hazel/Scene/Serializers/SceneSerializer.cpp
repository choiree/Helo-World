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

#include "Hazel/Serialization/ArchiveYAML.h"
#include "Hazel/Serialization/Serialization.h"

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
	//  Component name helper
	// ==================================================================

	template<typename T>
	static const char* ComponentName()
	{
		auto type = entt::resolve<T>();
		return type.name();
	}

	// ==================================================================
	//  Component type list
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

	// ==================================================================
	//  Branch-1: Component serialization specializations
	// ==================================================================

	template<>
	struct Serializer<CameraComponent>
	{
		static void Do(Archive& ar, CameraComponent& cc)
		{
			if (ar.GetMode() == Archive::Mode::Write)
			{
				auto& camera = cc.Camera;
				ar.BeginObject("Camera");
				int projType = (int)camera.GetProjectionType();
				ar.Value("ProjectionType", projType);
				float fov = camera.GetPerspectiveVerticalFOV();
				ar.Value("PerspectiveFOV", fov);
				float pNear = camera.GetPerspectiveNearClip();
				ar.Value("PerspectiveNear", pNear);
				float pFar = camera.GetPerspectiveFarClip();
				ar.Value("PerspectiveFar", pFar);
				float orthoSize = camera.GetOrthographicSize();
				ar.Value("OrthographicSize", orthoSize);
				float oNear = camera.GetOrthographicNearClip();
				ar.Value("OrthographicNear", oNear);
				float oFar = camera.GetOrthographicFarClip();
				ar.Value("OrthographicFar", oFar);
				ar.EndObject();
			}
			else
			{
				if (ar.HasKey("Camera"))
				{
					ar.BeginObject("Camera");
					int projType = 0;
					ar.Value("ProjectionType", projType);
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)projType);
					float fov = cc.Camera.GetPerspectiveVerticalFOV();
					ar.Value("PerspectiveFOV", fov);
					cc.Camera.SetPerspectiveVerticalFOV(fov);
					float pNear = cc.Camera.GetPerspectiveNearClip();
					ar.Value("PerspectiveNear", pNear);
					cc.Camera.SetPerspectiveNearClip(pNear);
					float pFar = cc.Camera.GetPerspectiveFarClip();
					ar.Value("PerspectiveFar", pFar);
					cc.Camera.SetPerspectiveFarClip(pFar);
					float orthoSize = cc.Camera.GetOrthographicSize();
					ar.Value("OrthographicSize", orthoSize);
					cc.Camera.SetOrthographicSize(orthoSize);
					float oNear = cc.Camera.GetOrthographicNearClip();
					ar.Value("OrthographicNear", oNear);
					cc.Camera.SetOrthographicNearClip(oNear);
					float oFar = cc.Camera.GetOrthographicFarClip();
					ar.Value("OrthographicFar", oFar);
					cc.Camera.SetOrthographicFarClip(oFar);
					ar.EndObject();
				}
			}

			auto type = entt::resolve<CameraComponent>();
			if (type) SerializeReflected(ar, type, cc);
		}
	};

	template<>
	struct Serializer<ScriptComponent>
	{
		static void Do(Archive& ar, ScriptComponent& sc)
		{
			auto type = entt::resolve<ScriptComponent>();
			if (type) SerializeReflected(ar, type, sc);
		}
	};

	template<>
	struct Serializer<SpriteRendererComponent>
	{
		static void Do(Archive& ar, SpriteRendererComponent& src)
		{
			if (ar.GetMode() == Archive::Mode::Write)
			{
				if (src.Texture)
				{
					std::string path = src.Texture->GetPath();
					ar.Value("TexturePath", path);
				}
				if (src.SubTexture)
				{
					ar.BeginObject("SubTexture");
					glm::vec2 uv0 = src.SubTexture->GetUV0();
					glm::vec2 uv1 = src.SubTexture->GetUV1();
					ar.Value("UV0", uv0);
					ar.Value("UV1", uv1);
					ar.EndObject();
				}
			}
			else
			{
				if (ar.HasKey("TexturePath"))
				{
					std::string texturePath;
					ar.Value("TexturePath", texturePath);
					src.Texture = AssetManager::Load<Texture2D>(texturePath);
				}
				if (ar.HasKey("SubTexture") && src.Texture)
				{
					ar.BeginObject("SubTexture");
					glm::vec2 uv0{}, uv1{};
					ar.Value("UV0", uv0);
					ar.Value("UV1", uv1);
					src.SubTexture = SubTexture2D::Create(src.Texture, uv0, uv1);
					ar.EndObject();
				}
			}

			auto type = entt::resolve<SpriteRendererComponent>();
			if (type) SerializeReflected(ar, type, src);
		}
	};

	template<>
	struct Serializer<Rigidbody2DComponent>
	{
		static void Do(Archive& ar, Rigidbody2DComponent& rb2d)
		{
			if (ar.GetMode() == Archive::Mode::Write)
			{
				std::string bodyType = RigidBody2DBodyTypeToString(rb2d.Type);
				ar.Value("BodyType", bodyType);
			}
			else
			{
				if (ar.HasKey("BodyType"))
				{
					std::string bodyTypeStr;
					ar.Value("BodyType", bodyTypeStr);
					rb2d.Type = RigidBody2DBodyTypeFromString(bodyTypeStr);
				}
			}

			auto type = entt::resolve<Rigidbody2DComponent>();
			if (type) SerializeReflected(ar, type, rb2d);
		}
	};

	template<>
	struct Serializer<SpriteAnimationComponent>
	{
		static void Do(Archive& ar, SpriteAnimationComponent& anim)
		{
			auto type = entt::resolve<SpriteAnimationComponent>();
			if (type) SerializeReflected(ar, type, anim);
		}
	};

	template<>
	struct Serializer<TileMapComponent>
	{
		static void Do(Archive& ar, TileMapComponent& tmc)
		{
			if (ar.GetMode() == Archive::Mode::Write)
			{
				if (tmc.Map)
				{
					std::string path = tmc.Map->GetSourcePath();
					ar.Value("MapPath", path);
				}
			}
			else
			{
				if (ar.HasKey("MapPath"))
				{
					std::string mapPath;
					ar.Value("MapPath", mapPath);
					tmc.Map = AssetManager::Load<TileMapAsset>(mapPath);
				}
			}

			auto type = entt::resolve<TileMapComponent>();
			if (type) SerializeReflected(ar, type, tmc);
		}
	};

	// ==================================================================
	//  Fold expression helpers for entity-level component iteration
	// ==================================================================

	template<typename... Ts>
	static void SerializeComponents(Archive& ar, Entity entity, std::tuple<Ts...>)
	{
		((entity.HasComponent<Ts>() && [&]() {
			ar.BeginObject(ComponentName<Ts>());
			Serialize(ar, entity.GetComponent<Ts>());
			ar.EndObject();
			return true;
		}()), ...);
	}

	template<typename... Ts>
	static void DeserializeComponents(Archive& ar, Entity entity, std::tuple<Ts...>)
	{
		((ar.HasKey(ComponentName<Ts>()) && [&]() {
			auto& comp = entity.AddOrReplaceComponent<Ts>();
			ar.BeginObject(ComponentName<Ts>());
			Serialize(ar, comp);
			ar.EndObject();
			return true;
		}()), ...);
	}

	// ==================================================================
	//  Entity serialization (Branch-1: Entity)
	// ==================================================================

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		HZ_CORE_ASSERT(entity.HasComponent<IDComponent>());

		out << YAML::BeginMap;
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		ArchiveYAML ar(out);
		SerializeComponents(ar, entity, SceneComponentTypes{});

		// ScriptComponent: ScriptFields (needs Entity + raw emitter)
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& sc = entity.GetComponent<ScriptComponent>();
			Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
			std::map<std::string, ScriptField> emptyFields;
			const auto& fields = entityClass ? entityClass->GetFields() : emptyFields;
			if (!fields.empty())
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
		}

		// SpriteAnimationComponent: Clips and CurrentClip
		if (entity.HasComponent<SpriteAnimationComponent>())
		{
			auto& anim = entity.GetComponent<SpriteAnimationComponent>();
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
		}

		out << YAML::EndMap;
	}

	// ==================================================================
	//  Entity deserialization
	// ==================================================================

	static void DeserializeEntity(const YAML::Node& entityNode, Entity entity)
	{
		ArchiveYAML ar(entityNode);
		DeserializeComponents(ar, entity, SceneComponentTypes{});

		// ScriptComponent: restore ScriptFields
		auto scriptNode = entityNode["ScriptComponent"];
		if (scriptNode)
		{
			auto scriptFieldsNode = scriptNode["ScriptFields"];
			if (scriptFieldsNode && entity.HasComponent<ScriptComponent>())
			{
				auto& sc = entity.GetComponent<ScriptComponent>();
				Ref<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
				if (entityClass)
				{
					const auto& fields = entityClass->GetFields();
					auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);
					for (auto scriptField : scriptFieldsNode)
					{
						std::string name = scriptField["Name"].as<std::string>();
						std::string typeString = scriptField["Type"].as<std::string>();
						ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);
						ScriptFieldInstance& fieldInstance = entityFields[name];
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

		// SpriteAnimationComponent: reconstruct Clips
		if (entity.HasComponent<SpriteAnimationComponent>())
		{
			auto& anim = entity.GetComponent<SpriteAnimationComponent>();
			auto animNode = entityNode["SpriteAnimationComponent"];
			auto clipsNode = animNode["Clips"];
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

			if (animNode["CurrentClip"])
			{
				std::string currentClipName = animNode["CurrentClip"].as<std::string>();
				auto it = anim.Clips.find(currentClipName);
				if (it != anim.Clips.end())
					anim.CurrentClip = it->second;
			}
		}
	}

	// ==================================================================
	//  SceneSerializer public API
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
			for (auto entityNode : entities)
			{
				uint64_t uuid = entityNode["Entity"].as<uint64_t>();

				std::string name;
				auto tagNode = entityNode["TagComponent"];
				if (tagNode)
					name = tagNode["Tag"].as<std::string>();

				HZ_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				DeserializeEntity(entityNode, deserializedEntity);
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		HZ_CORE_ASSERT(false);
		return false;
	}

}
