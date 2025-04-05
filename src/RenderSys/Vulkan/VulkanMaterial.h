#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Material.h>

namespace RenderSys
{

class VulkanMaterialDescriptor
{
public:
    VulkanMaterialDescriptor(MaterialTextures& textures);
    ~VulkanMaterialDescriptor();

    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
};


} // namespace RenderSys