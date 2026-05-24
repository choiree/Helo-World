#pragma once

#include "Hazel/Scene/System.h"
#include "Hazel/Scene/SceneCamera.h"

#include <glm/glm.hpp>

namespace Hazel {

	class EditorCamera;

	// Stored in entt::registry::ctx for camera↔render communication
	struct SceneRenderContext
	{
		Camera* RuntimeCamera = nullptr;
		glm::mat4 CameraTransform{ 1.0f };
		EditorCamera* EditorCamera = nullptr;
		bool UseEditorCamera = false;
	};

}
