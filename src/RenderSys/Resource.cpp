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

} // namespace RenderSys