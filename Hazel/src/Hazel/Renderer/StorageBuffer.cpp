#include "hzpch.h"
#include "StorageBuffer.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLStorageBuffer.h"

namespace Hazel {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLStorageBuffer>(size, binding);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
