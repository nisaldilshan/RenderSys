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

Buffer::Buffer(size_t byteSize, RenderSys::BufferUsage bufferUsage)
    : m_platformBuffer(std::make_shared<BufferType>(byteSize, bufferUsage))
{

}

Buffer::~Buffer()
{
}

void Buffer::MapBuffer()
{
    m_platformBuffer->MapBuffer();
}

void Buffer::WriteToBuffer(const void *data)
{
    assert(data != nullptr);
    m_platformBuffer->WriteToBuffer(data);
}

bool Buffer::Flush()
{
    return false;
}

} // namespace RenderSys