#include "WalnutAdapter.h"

#if (RENDERSYS_BACKEND == 2)
#include <RenderSys/Vulkan/VulkanContext.h>
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>
#endif

namespace RenderSys
{

void AttachToWalnut()
{
#if (RENDERSYS_BACKEND == 2)
    Vulkan::Context context;
    context.instance = GraphicsAPI::Vulkan::GetInstance();
    context.physicalDevice = GraphicsAPI::Vulkan::GetPhysicalDevice();
    context.device = GraphicsAPI::Vulkan::GetDevice();
    context.queueFamily = GraphicsAPI::Vulkan::GetQueueFamily();
    context.queue = GraphicsAPI::Vulkan::GetQueue();
    context.uiDescriptorPool = GraphicsAPI::Vulkan::GetDescriptorPool();
    context.allocator = GraphicsAPI::Vulkan::GetAllocator();
    Vulkan::SetContext(context);
#elif (RENDERSYS_BACKEND == 3)
    // The WebGPU backend still takes its device straight from Walnut; nothing to attach yet
#else
    static_assert(false, "AttachToWalnut does not support this backend");
#endif
}

} // namespace RenderSys
