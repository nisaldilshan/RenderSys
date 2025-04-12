#include "Buffer.h"

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanBuffer.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPUBuffer.h"
#else
static_assert(false);
#endif

namespace RenderSys
{

Buffer::Buffer(uint32_t byteSize, RenderSys::BufferUsage bufferUsage)
{
}

} // namespace RenderSys