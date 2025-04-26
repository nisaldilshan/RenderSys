#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Material.h>

namespace RenderSys
{

void CreateMaterialBindGroupPool();
VkDescriptorPool GetMaterialBindGroupPool();
void DestroyMaterialBindGroupPool();

void CreateMaterialBindGroupLayout();
VkDescriptorSetLayout GetMaterialBindGroupLayout();
void DestroyMaterialBindGroupLayout();

class VulkanMaterialDescriptor
{
public:
    VulkanMaterialDescriptor(MaterialTextures& textures);
    ~VulkanMaterialDescriptor();

    // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; 
};


} // namespace RenderSys