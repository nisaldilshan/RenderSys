#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace RenderSys
{
namespace Vulkan
{

// Every Vulkan handle RenderSys records and submits with. A host fills it once,
// before any renderer is created: the Walnut adapter wraps Walnut's device, and a
// game embedding RenderSys hands over its own.
struct Context
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkQueue queue = VK_NULL_HANDLE;
    // Pool the host's UI draws from, used to show render targets as ImGui images. Optional.
    VkDescriptorPool uiDescriptorPool = VK_NULL_HANDLE;
    const VkAllocationCallbacks* allocator = nullptr;
};

void SetContext(const Context& context);
const Context& GetContext();

VkInstance GetInstance();
VkPhysicalDevice GetPhysicalDevice();
VkDevice GetDevice();
uint32_t GetQueueFamily();
VkQueue GetQueue();

void CheckVkResult(VkResult err);
// Submits to the context's queue and blocks until the GPU has finished.
void QueueSubmitAndWait(const VkSubmitInfo& submitInfo);
uint32_t FindMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);

} // namespace Vulkan
} // namespace RenderSys
