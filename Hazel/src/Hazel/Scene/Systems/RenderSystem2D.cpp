#include "hzpch.h"
#include "RenderSystem2D.h"

#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/SceneRenderContext.h"

namespace Hazel {

	void RenderSystem2D::Execute(entt::registry& registry, Timestep ts)
	{
		auto& ctx = registry.ctx().get<SceneRenderContext&>();

		if (ctx.UseEditorCamera)
		{
			if (!ctx.EditorCamera)
				return;
			Renderer2D::BeginScene(*ctx.EditorCamera);
		}
		else
		{
			if (!ctx.RuntimeCamera)
				return;
			Renderer2D::BeginScene(*ctx.RuntimeCamera, ctx.CameraTransform);
		}

		// Draw sprites
		{
			auto group = registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto [entity, transform, sprite] : group.each())
				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
		}

		// Draw circles
		{
			auto view = registry.view<TransformComponent, CircleRendererComponent>();
			for (auto [entity, transform, circle] : view.each())
				Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
		}

		// Draw text
		{
			auto view = registry.view<TransformComponent, TextComponent>();
			for (auto [entity, transform, text] : view.each())
				Renderer2D::DrawString(text.TextString, transform.GetTransform(), text, (int)entity);
		}

		Renderer2D::EndScene();
	}

}
