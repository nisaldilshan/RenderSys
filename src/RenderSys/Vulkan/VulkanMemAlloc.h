#pragma once
#include <vk_mem_alloc.h>

namespace RenderSys
{
namespace Vulkan 
{

VmaAllocator GetMemoryAllocator();
void DestroyMemoryAllocator();

} // namespace Vulkan
} // namespace RenderSys