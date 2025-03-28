#include "VulkanMemAlloc.h"
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <iostream>

namespace RenderSys
{
namespace Vulkan 
{

VmaAllocator g_allocator = VK_NULL_HANDLE;

VmaAllocator GetMemoryAllocator()
{
    if (g_allocator == VK_NULL_HANDLE)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = GraphicsAPI::Vulkan::GetPhysicalDevice();
        allocatorInfo.device = GraphicsAPI::Vulkan::GetDevice();
        allocatorInfo.instance = GraphicsAPI::Vulkan::GetInstance();
        if (vmaCreateAllocator(&allocatorInfo, &g_allocator) != VK_SUCCESS) {
            std::cout << "error: could not init VMA" << std::endl;
            assert(false);
        }
        std::cout << "VMACreateAllocator - done!" << std::endl;
    }
    return g_allocator;
}

void DestroyMemoryAllocator()
{
    if (g_allocator != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(g_allocator);
        g_allocator = VK_NULL_HANDLE;
    }
}

} // namespace Vulkan
} // namespace RenderSys