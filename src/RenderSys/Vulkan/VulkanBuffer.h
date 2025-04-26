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
    void UnmapBuffer();
    void WriteToBuffer(const void *data);
    bool Flush();
    const VkDescriptorBufferInfo& GetBufferInfo() const;

private:
    VkDeviceSize m_bufferSize = 0;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_bufferMemory = VK_NULL_HANDLE;
    void* m_mapped = nullptr;
    VkDescriptorBufferInfo m_bufferInfo = {VK_NULL_HANDLE, 0, 0};
};

} // namespace RenderSys