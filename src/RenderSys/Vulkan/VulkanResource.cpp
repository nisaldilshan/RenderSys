#include "VulkanResource.h"

namespace RenderSys
{

VulkanResourceDescriptor::VulkanResourceDescriptor()
{

    
}

VulkanResourceDescriptor::~VulkanResourceDescriptor()
{
}

void VulkanResourceDescriptor::AttachBuffer(uint32_t binding, const VkDescriptorBufferInfo &bufferInfo)
{
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_bindGroup;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferInfo;

    m_Writes.push_back(write);
}

void VulkanResourceDescriptor::Init()
{
    vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), m_Writes.size(), m_Writes.data(), 0, nullptr);
}

} // namespace RenderSys