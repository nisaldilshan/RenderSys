#include "VulkanBuffer.h"

namespace RenderSys
{

VulkanBuffer::VulkanBuffer(uint32_t byteSize, RenderSys::BufferUsage bufferUsage)
{
    VkBufferCreateInfo bufferInfo{};
    VmaAllocationCreateInfo allocInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (bufferUsage == RenderSys::BufferUsage::UNIFORM_BUFFER_VISIBLE_TO_CPU) 
    {
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    } 
    else if (bufferUsage == RenderSys::BufferUsage::STORAGE_BUFFER_VISIBLE_TO_CPU) 
    {
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    } 
    else if (bufferUsage == RenderSys::BufferUsage::TRANSFER_SRC_VISIBLE_TO_GPU) 
    {
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    } 
    else 
    {
        assert(false);
    }
    
    if (vmaCreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferAllocation, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }
}

VulkanBuffer::~VulkanBuffer()
{
}

} // namespace RenderSys