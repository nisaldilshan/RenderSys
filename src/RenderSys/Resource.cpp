#include "Resource.h"

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanResource.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPUResource.h"
#else
static_assert(false);
#endif

namespace RenderSys
{


ResourceDescriptor::ResourceDescriptor()
    : m_platformDescriptor(std::make_unique<ResourceDescriptorType>())
{
}

ResourceDescriptor::~ResourceDescriptor()
{

}

void ResourceDescriptor::AttachBuffer(uint32_t binding, const std::shared_ptr<Buffer> &buffer)
{
    m_platformDescriptor->AttachBuffer(binding, buffer->GetPlatformBuffer()->GetBufferInfo());
}

void ResourceDescriptor::Init()
{
    m_platformDescriptor->Init();
}

Resource::Resource()
    : m_ResourceDescriptor(std::make_shared<ResourceDescriptor>())
{
}

Resource::~Resource()
{
}

void Resource::SetBuffer(BufferIndices index, const std::shared_ptr<Buffer> &buffer)
{
    assert(index < NUM_BUFFERS);
    m_ResourceBuffers[index] = buffer;

    if (index == INSTANCE_BUFFER_INDEX)
    {
        m_ResourceDescriptor->AttachBuffer(0, buffer);
    }
    // else if (index == SKELETAL_ANIMATION_BUFFER_INDEX)
    // {
    //     m_ResourceDescriptor->AttachBuffer(1, buffer);
    // }
    // else if (index == HEIGHTMAP)
    // {
    //     m_ResourceDescriptor->AttachBuffer(2, buffer);
    // }
    // else if (index == MULTI_PURPOSE_BUFFER)
    // {
    //     m_ResourceDescriptor->AttachBuffer(3, buffer);
    // }
    else
    {
        assert(false);
    }
}

void Resource::Init()
{
    m_ResourceDescriptor->Init();
}

} // namespace RenderSys