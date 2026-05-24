#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"

#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "AnimationSystem.h"
#include "Hazel/Physics/Physics2D.h"

#include "Systems/ScriptSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/TransformSyncSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/RenderSystem2D.h"

// Box2D
#include "box2d/box2d.h"

namespace Hazel {

	Scene::Scene()
		: m_PhysicsWorldId(b2_nullWorldId)
	{
		m_Registry.ctx().emplace<SceneRenderContext>();

		m_SystemGraph.AddSystem<ScriptSystem>(this);
		m_SystemGraph.AddSystem<AnimationSystem>();
		m_SystemGraph.AddSystem<PhysicsSystem>(m_PhysicsWorldId);
		m_SystemGraph.AddSystem<TransformSyncSystem>();
		m_SystemGraph.AddSystem<CameraSystem>();
		m_SystemGraph.AddSystem<RenderSystem2D>();
		m_SystemGraph.Build();
	}

	Scene::~Scene()
	{
		m_SystemGraph.OnDetach(m_Registry);

		if (b2World_IsValid(m_PhysicsWorldId))
			b2DestroyWorld(m_PhysicsWorldId);
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto [srcEntity, srcComponent] : view.each())
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent, TagComponent>();
		for (auto [e, idComp, tagComp] : idView.each())
		{
			UUID uuid = idComp.ID;
			const auto& name = tagComp.Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	void Scene::OnRuntimeStart()
	{
		m_IsRunning = true;

		OnPhysics2DStart();

		m_SystemGraph.OnAttach(m_Registry);

		// Scripting
		{
			ScriptEngine::OnRuntimeStart(this);
			// Instantiate all script entities

			auto view = m_Registry.view<ScriptComponent>();
			for (auto [e, _] : view.each())
			{
				Entity entity = { e, this };
				ScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		m_IsRunning = false;

		m_SystemGraph.OnDetach(m_Registry);

		OnPhysics2DStop();

		ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnSimulationStart()
	{
		OnPhysics2DStart();
	}

	void Scene::OnSimulationStop()
	{
		OnPhysics2DStop();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			m_SystemGraph.ExecuteStage(SystemStage::PreUpdate,   m_Registry, ts);
			m_SystemGraph.ExecuteStage(SystemStage::Update,      m_Registry, ts);
			m_SystemGraph.ExecuteStage(SystemStage::Physics,     m_Registry, ts);
			m_SystemGraph.ExecuteStage(SystemStage::PostPhysics, m_Registry, ts);
		}

		// Render always runs (even when paused)
		m_Registry.ctx().get<SceneRenderContext&>().UseEditorCamera = false;
		m_SystemGraph.ExecuteStage(SystemStage::PreRender, m_Registry, ts);
		m_SystemGraph.ExecuteStage(SystemStage::Render,    m_Registry, ts);
	}

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& camera)
	{
		if (!m_IsPaused || m_StepFrames-- > 0)
		{
			m_SystemGraph.ExecuteStage(SystemStage::Physics,     m_Registry, ts);
			m_SystemGraph.ExecuteStage(SystemStage::PostPhysics, m_Registry, ts);
		}

		auto& ctx = m_Registry.ctx().get<SceneRenderContext&>();
		ctx.UseEditorCamera = true;
		ctx.EditorCamera = &camera;

		m_SystemGraph.ExecuteStage(SystemStage::Render, m_Registry, ts);
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		m_SystemGraph.ExecuteStage(SystemStage::Update, m_Registry, ts);

		auto& ctx = m_Registry.ctx().get<SceneRenderContext&>();
		ctx.UseEditorCamera = true;
		ctx.EditorCamera = &camera;

		m_SystemGraph.ExecuteStage(SystemStage::Render, m_Registry, ts);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto [entity, cameraComponent] : view.each())
		{
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto [entity, camera] : view.each())
		{
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto [entity, tc] : view.each())
		{
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}

	void Scene::OnPhysics2DStart()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0.0f, -9.8f };
		worldDef.restitutionThreshold = 0.5f;
		m_PhysicsWorldId = b2CreateWorld(&worldDef);

		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto [entity, rb2d] : view.each())
		{
			Entity e = { entity, this };
			auto& transform = e.GetComponent<TransformComponent>();

			b2BodyDef bodyDef = b2DefaultBodyDef();
			bodyDef.type = Utils::Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position = { transform.Translation.x, transform.Translation.y };
			bodyDef.rotation = b2MakeRot(transform.Rotation.z);
			bodyDef.motionLocks.angularZ = rb2d.FixedRotation;

			b2BodyId bodyId = b2CreateBody(m_PhysicsWorldId, &bodyDef);
			rb2d.RuntimeBody = bodyId;

			// BoxCollider2D
			if (e.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = e.GetComponent<BoxCollider2DComponent>();

				b2ShapeDef shapeDef = b2DefaultShapeDef();
				shapeDef.density = bc2d.Density;
				shapeDef.material.friction = bc2d.Friction;
				shapeDef.material.restitution = bc2d.Restitution;

				b2Polygon polygon = b2MakeBox(bc2d.Size.x * transform.Scale.x,
					bc2d.Size.y * transform.Scale.y);
				polygon.centroid = { bc2d.Offset.x, bc2d.Offset.y };

				bc2d.RuntimeFixture = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
			}

			// CircleCollider2D
			if (e.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = e.GetComponent<CircleCollider2DComponent>();

				b2ShapeDef shapeDef = b2DefaultShapeDef();
				shapeDef.density = cc2d.Density;
				shapeDef.material.friction = cc2d.Friction;
				shapeDef.material.restitution = cc2d.Restitution;

				b2Circle circle;
				circle.center = { cc2d.Offset.x, cc2d.Offset.y };
				circle.radius = transform.Scale.x * cc2d.Radius;

				cc2d.RuntimeFixture = b2CreateCircleShape(bodyId, &shapeDef, &circle);
			}
		}
	}

	void Scene::OnPhysics2DStop()
	{
		if (b2World_IsValid(m_PhysicsWorldId))
			b2DestroyWorld(m_PhysicsWorldId);
		m_PhysicsWorldId = B2_NULL_ID;
	}

	template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		// Default no-op — registered components handle init through meta hooks if needed.
		(void)entity;
		(void)component;
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}
	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

}
