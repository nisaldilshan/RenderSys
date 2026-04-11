#pragma once

#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>

#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>
#include <RenderSys/Vulkan/VulkanVertex.h>
#include <RenderSys/Vulkan/VulkanRendererUtils.h>

namespace RenderSys
{
namespace Vulkan
{
class Render2DPipeline;
} // namespace Vulkan
class VulkanRenderer2D
{
public:
    VulkanRenderer2D();
    ~VulkanRenderer2D();

    bool Init();
    void CreateTextureToRenderInto(uint32_t width, uint32_t height);
    void CreateTextureSampler();
    void CreateShaders(RenderSys::Shader& shader);
    void CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
    void CreatePipeline();
    void CreateFrameBuffer();
    void CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
    void CreateIndexBuffer(const std::vector<uint16_t> &bufferData);
    void CreateBindGroup(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry);
    void CreateUniformBuffer(size_t uniformCountInBuffer, uint32_t sizeOfOneUniform);
    void SetUniformData(const void* bufferData, uint32_t uniformIndex);
    void SimpleRender();
    void Render();
    void RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount);
    uint64_t GetDescriptorSet();
    void BeginRenderPass();
    void EndRenderPass();
    void Destroy();
private:
    bool CreateRenderPass();
    void DestroyBuffers();
    void DestroyShaders();
    void SubmitCommandBuffer();

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    VkImage m_ImageToRenderInto = VK_NULL_HANDLE;
    std::unique_ptr<Vulkan::RenderTarget> m_finalRenderTarget;
    VkSampler m_textureSampler;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
    std::unordered_map<std::string, std::vector<uint32_t>> m_shaderMap;

    std::unique_ptr<Vulkan::Render2DPipeline> m_render2DPipeline;
    VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
    VkRenderPass m_renderpass = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    Vulkan::VertexInputLayout m_vertexInputLayout;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferMemory = VK_NULL_HANDLE;
    std::vector<VkVertexInputBindingDescription> m_vertextBindingDescs;
    std::vector<VkVertexInputAttributeDescription> m_vertextAttribDescs;
    uint32_t m_vertexCount = 0;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_indexCount = 0;

    VkDescriptorSetLayout m_bindGroupLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_bindGroupPool = VK_NULL_HANDLE;
    VkDescriptorSet m_bindGroup = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> m_bindGroupBindings;
    uint32_t m_sizeOfOneUniform = 0;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VmaAllocation> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    VmaAllocator m_vma = VK_NULL_HANDLE;
};

}