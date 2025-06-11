#pragma once
#include <vk_mem_alloc.h>

namespace RenderSys
{
namespace Vulkan
{

struct VertexInputLayout
{
    VkVertexInputBindingDescription m_vertextBindingDescs;
    std::vector<VkVertexInputAttributeDescription> m_vertextAttribDescs;
};

struct VertexIndexBufferInfo
{
    VertexIndexBufferInfo() = default;
    ~VertexIndexBufferInfo() = default;
    VertexIndexBufferInfo(const VertexIndexBufferInfo&) = delete;
    VertexIndexBufferInfo& operator=(const VertexIndexBufferInfo&) = delete;
    VertexIndexBufferInfo(VertexIndexBufferInfo&&) = delete;
    VertexIndexBufferInfo& operator=(VertexIndexBufferInfo&&) = delete;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_vertexCount = 0;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_indexCount = 0;
};

} // namespace Vulkan

} // namespace RenderSys
