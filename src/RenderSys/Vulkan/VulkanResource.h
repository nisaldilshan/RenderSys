#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Resource.h>

namespace RenderSys
{

class VulkanResourceDescriptor
{
public:
    VulkanResourceDescriptor();
    ~VulkanResourceDescriptor();

    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; 
};


} // namespace RenderSys