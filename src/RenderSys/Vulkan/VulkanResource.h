#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Resource.h>
#include "VulkanBuffer.h"
namespace RenderSys
{

void CreateResourceBindGroupPool();
VkDescriptorPool GetResourceBindGroupPool();
void DestroyResourceBindGroupPool();

void CreateResourceBindGroupLayout();
VkDescriptorSetLayout GetResourceBindGroupLayout();
void DestroyResourceBindGroupLayout();

class VulkanResourceDescriptor
{
public:
    VulkanResourceDescriptor();
    ~VulkanResourceDescriptor();

    void AttachBuffer(uint32_t binding, const VkDescriptorBufferInfo& bufferInfo);
    void Init();

    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; 
private:
    std::vector<VkWriteDescriptorSet> m_Writes;
};


} // namespace RenderSys