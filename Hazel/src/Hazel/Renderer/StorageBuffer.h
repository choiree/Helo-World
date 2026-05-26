#pragma once

#include "Hazel/Core/Base.h"

namespace Hazel {

	class StorageBuffer
	{
	public:
		virtual ~StorageBuffer() {}
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind(uint32_t binding) = 0;

		static Ref<StorageBuffer> Create(uint32_t size, uint32_t binding);
	};

}
