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
private:
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_bufferAllocation = VK_NULL_HANDLE;
};

} // namespace RenderSys