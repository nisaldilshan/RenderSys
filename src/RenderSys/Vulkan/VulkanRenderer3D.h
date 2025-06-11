#pragma once

#include <array>
#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>
#include <RenderSys/Buffer.h>
#include <RenderSys/Texture.h>
#include <RenderSys/Scene/Mesh.h>

namespace RenderSys
{
namespace Vulkan
{

class ShadowMap;
class PbrRenderPipeline;

struct VertexInputLayout
{
    VkVertexInputBindingDescription m_vertextBindingDescs;
    std::vector<VkVertexInputAttributeDescription> m_vertextAttribDescs;
};

} // namespace Vulkan

struct VulkanVertexIndexBufferInfo
{
    VulkanVertexIndexBufferInfo() = default;
    ~VulkanVertexIndexBufferInfo() = default;
    VulkanVertexIndexBufferInfo(const VulkanVertexIndexBufferInfo&) = delete;
    VulkanVertexIndexBufferInfo& operator=(const VulkanVertexIndexBufferInfo&) = delete;
    VulkanVertexIndexBufferInfo(VulkanVertexIndexBufferInfo&&) = delete;
    VulkanVertexIndexBufferInfo& operator=(VulkanVertexIndexBufferInfo&&) = delete;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_vertexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_vertexCount = 0;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VmaAllocation m_indexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_indexCount = 0;
};

struct VulkanUniformBufferInfo
{
    VkDescriptorBufferInfo m_bufferInfo = {VK_NULL_HANDLE, 0, 0};
    VmaAllocation m_uniformBufferMemory = VK_NULL_HANDLE;
    void* m_mappedBuffer = nullptr;
};

class VulkanRenderer3D
{
public:
    VulkanRenderer3D();
    ~VulkanRenderer3D();

    bool Init();
    void CreateImageToRender(uint32_t width, uint32_t height);
    void CreateDepthImage();
    void CreateShaders(RenderSys::Shader& shader);
    void CreatePipeline();
    void CreateFrameBuffer();
    uint32_t CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout);
    void CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData);
    void SetClearColor(glm::vec4 clearColor);
    void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform);
    // Textures get created as a part of main bindgroup
    void CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture);
    void SetUniformData(uint32_t binding, const void* bufferData);
    void BindResources();
    void Render();
    void RenderIndexed();
    void RenderMesh(const RenderSys::Mesh& mesh);
    void DrawPlane();
    void DrawCube();
    ImTextureID GetDescriptorSet();
    void BeginRenderPass();
    void EndRenderPass();
    void BeginShadowMapPass();
    void EndShadowMapPass();
    void DestroyImages();
    void DestroyPipeline();
    void DestroyBindGroup();
    void Destroy();
    void ResetCommandBuffer();
    void SubmitCommandBuffer();

private:
    void RenderSubMesh(const uint32_t vertexBufferID, const RenderSys::SubMesh& subMesh);
    void CreateDefaultTextureSampler();
    void CreatePipelineLayout();
    void CreateRenderPass();
    void CreateCommandBuffers();
    void DestroyRenderPass();
    void DestroyBuffers();
    void DestroyShaders();
    void DestroyTextures();
    

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    VkImage m_ImageToRenderInto = VK_NULL_HANDLE;
    VmaAllocation m_renderImageMemory = VK_NULL_HANDLE;
    VkImageView m_imageViewToRenderInto = VK_NULL_HANDLE;
    VkImage m_depthimage = VK_NULL_HANDLE;
    VmaAllocation m_depthimageMemory = VK_NULL_HANDLE;
    VkImageView m_depthimageView = VK_NULL_HANDLE;

    VkSampler m_defaultTextureSampler = VK_NULL_HANDLE;
    VkDescriptorSet m_finalImageDescriptorSet = VK_NULL_HANDLE;
    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
    std::unordered_map<std::string, std::vector<uint32_t>> m_shaderMap;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    std::unique_ptr<Vulkan::PbrRenderPipeline> m_pbrRenderPipeline;

    VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
    VkRenderPass m_renderpass = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    Vulkan::VertexInputLayout m_vertexInputLayout;
    std::unordered_map<uint32_t, std::shared_ptr<VulkanVertexIndexBufferInfo>> m_vertexIndexBufferInfoMap;

    VkDescriptorPool m_bindGroupPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_bindGroupLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_mainBindGroup = VK_NULL_HANDLE;

    std::unordered_map<uint32_t, std::shared_ptr<RenderSys::VulkanTexture>> m_textures;
    // map bindingNumber to tuple -> <VkDescriptorBufferInfo, uniformBufferMemory, mappedBuffer>
    std::unordered_map<uint32_t, std::tuple<VkDescriptorBufferInfo, VmaAllocation, void*>> m_uniformBuffers;
    VkClearColorValue m_clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

    std::shared_ptr<RenderSys::Vulkan::ShadowMap> m_shadowMap;
};

}