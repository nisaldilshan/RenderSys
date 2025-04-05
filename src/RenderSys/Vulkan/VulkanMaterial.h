#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

namespace RenderSys
{

class VulkanMaterialDescriptor
{
public:
    VulkanMaterialDescriptor();
    ~VulkanMaterialDescriptor();

    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
};


} // namespace RenderSys