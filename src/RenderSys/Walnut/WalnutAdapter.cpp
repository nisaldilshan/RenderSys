#include "WalnutAdapter.h"
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>

namespace RenderSys
{
namespace Vulkan
{

Context MakeContextFromWalnut()
{
    Context context;
    context.instance = GraphicsAPI::Vulkan::GetInstance();
    context.physicalDevice = GraphicsAPI::Vulkan::GetPhysicalDevice();
    context.device = GraphicsAPI::Vulkan::GetDevice();
    context.queueFamily = GraphicsAPI::Vulkan::GetQueueFamily();
    context.queue = GraphicsAPI::Vulkan::GetQueue();
    context.uiDescriptorPool = GraphicsAPI::Vulkan::GetDescriptorPool();
    context.allocator = GraphicsAPI::Vulkan::GetAllocator();
    return context;
}

} // namespace Vulkan
} // namespace RenderSys
