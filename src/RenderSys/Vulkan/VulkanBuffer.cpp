#include "VulkanBuffer.h"

namespace RenderSys
{

VulkanBuffer::VulkanBuffer(uint32_t byteSize, RenderSys::BufferUsage bufferUsage)
    : m_bufferSize(byteSize)
{
    VkBufferCreateInfo bufferInfo{};
    VmaAllocationCreateInfo allocInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = m_bufferSize;
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
    
    if (vmaCreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), &bufferInfo, &allocInfo, &m_buffer, &m_bufferMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }

    m_bufferInfo.buffer = m_buffer;
    m_bufferInfo.offset = 0;
    m_bufferInfo.range = m_bufferSize;
}

VulkanBuffer::~VulkanBuffer()
{
    UnmapBuffer();
    if (m_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(RenderSys::Vulkan::GetMemoryAllocator(), m_buffer, m_bufferMemory);
        m_buffer = VK_NULL_HANDLE;
        m_bufferMemory = VK_NULL_HANDLE;
    }
}

void VulkanBuffer::MapBuffer()
{
    auto res = vmaMapMemory(RenderSys::Vulkan::GetMemoryAllocator(), m_bufferMemory, &m_mapped);
    if (res != VK_SUCCESS) {
        assert(false);
    }
    assert(m_mapped != nullptr);
}

void VulkanBuffer::UnmapBuffer()
{
    if (m_mapped == nullptr) {
        return;
    }
    vmaUnmapMemory(RenderSys::Vulkan::GetMemoryAllocator(), m_bufferMemory);
    m_mapped = nullptr;
}

void VulkanBuffer::WriteToBuffer(const void *data)
{
    assert(m_mapped != nullptr);
    std::memcpy(m_mapped, data, m_bufferSize);
}

bool VulkanBuffer::Flush()
{
    return false;
}

const VkDescriptorBufferInfo& VulkanBuffer::GetBufferInfo() const
{
    return m_bufferInfo;
}

} // namespace RenderSys