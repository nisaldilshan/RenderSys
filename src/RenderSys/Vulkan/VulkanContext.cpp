#include "VulkanContext.h"

#ifdef RENDERSYS_WALNUT_ADAPTER
#include <RenderSys/Walnut/WalnutAdapter.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>

namespace RenderSys
{
namespace Vulkan
{

namespace
{
Context g_context;
}

void SetContext(const Context& context)
{
    assert(context.device != VK_NULL_HANDLE && context.queue != VK_NULL_HANDLE);
    g_context = context;
}

const Context& GetContext()
{
    if (g_context.device == VK_NULL_HANDLE)
    {
#ifdef RENDERSYS_WALNUT_ADAPTER
        // Walnut-hosted apps never set a context themselves, so adopt Walnut's device on first use.
        SetContext(MakeContextFromWalnut());
#else
        assert(false && "RenderSys::Vulkan::SetContext must be called before using RenderSys");
#endif
    }
    return g_context;
}

VkInstance GetInstance()
{
    return GetContext().instance;
}

VkPhysicalDevice GetPhysicalDevice()
{
    return GetContext().physicalDevice;
}

VkDevice GetDevice()
{
    return GetContext().device;
}

uint32_t GetQueueFamily()
{
    return GetContext().queueFamily;
}

VkQueue GetQueue()
{
    return GetContext().queue;
}

void CheckVkResult(VkResult err)
{
    if (err == VK_SUCCESS)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void QueueSubmitAndWait(const VkSubmitInfo& submitInfo)
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    auto err = vkCreateFence(GetDevice(), &fenceCreateInfo, nullptr, &fence);
    CheckVkResult(err);

    err = vkQueueSubmit(GetQueue(), 1, &submitInfo, fence);
    CheckVkResult(err);

    constexpr uint64_t fenceTimeoutNs = 100'000'000'000;
    err = vkWaitForFences(GetDevice(), 1, &fence, VK_TRUE, fenceTimeoutNs);
    CheckVkResult(err);

    vkDestroyFence(GetDevice(), fence, nullptr);
}

uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(GetPhysicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties && (typeBits & (1 << i)))
            return i;
    }
    return 0xffffffff;
}

} // namespace Vulkan
} // namespace RenderSys
