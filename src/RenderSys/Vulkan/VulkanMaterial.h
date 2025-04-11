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

    // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
    VkDescriptorSet m_materialbindGroup = VK_NULL_HANDLE; 
};


} // namespace RenderSys