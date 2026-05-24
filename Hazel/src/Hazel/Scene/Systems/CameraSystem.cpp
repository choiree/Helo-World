#include "hzpch.h"
#include "CameraSystem.h"

#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/SceneRenderContext.h"

namespace Hazel {

	void CameraSystem::Execute(entt::registry& registry, Timestep ts)
	{
		auto& ctx = registry.ctx().get<SceneRenderContext&>();

		// Skip if editor camera is already set
		if (ctx.UseEditorCamera)
			return;

		auto view = registry.view<TransformComponent, CameraComponent>();
		for (auto [entity, transform, camera] : view.each())
		{
			if (camera.Primary)
			{
				ctx.RuntimeCamera = &camera.Camera;
				ctx.CameraTransform = transform.GetTransform();
				return;
			}
		}

		ctx.RuntimeCamera = nullptr;
	}

}
