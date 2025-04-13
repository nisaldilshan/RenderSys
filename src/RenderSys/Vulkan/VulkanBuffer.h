#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Buffer.h>
#include "VulkanMemAlloc.h"

namespace RenderSys
{

class VulkanBuffer
{
public:
    VulkanBuffer(uint32_t byteSize, RenderSys::BufferUsage bufferUsage);
    ~VulkanBuffer();

    void MapBuffer();
    void WriteToBuffer(const void *data);
    bool Flush();
    const VkDescriptorBufferInfo& GetBufferInfo() const { return m_bufferInfo; }

private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_bufferAllocation = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_bufferInfo = {VK_NULL_HANDLE, 0, 0};
};

} // namespace RenderSys