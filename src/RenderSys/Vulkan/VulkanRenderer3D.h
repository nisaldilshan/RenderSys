#pragma once

#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include "../RenderUtil.h"
#include "../Shader.h"
#include "../Buffer.h"

namespace GraphicsAPI
{
    class VulkanRenderer3D
    {
    public:
        VulkanRenderer3D() = default;
        ~VulkanRenderer3D() = default;

        bool Init();
        void CreateTextureToRenderInto(uint32_t width, uint32_t height);
        void CreateDepthImage();
        void CreateTextureSampler();
        void CreateShaders(RenderSys::Shader& shader);
        void CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
        void CreatePipeline();
        void CreateFrameBuffer();
        void CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(const std::vector<uint16_t> &bufferData);
        void SetClearColor(glm::vec4 clearColor);
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform, uint32_t uniformCountInBuffer);
        void CreateTexture(uint32_t textureWidth, uint32_t textureHeight, const void* textureData, uint32_t mipMapLevelCount);
        void SetUniformData(uint32_t binding, const void* bufferData, uint32_t uniformIndex);
        void BindResources();
        void Render(uint32_t uniformIndex);
        void RenderIndexed(uint32_t uniformIndex);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void Destroy();
    private:
        void CreateBindGroup();
        void CreatePipelineLayout();
        bool CreateRenderPass();
        void DestroyBuffers();
        void DestroyShaders();
        void UploadTexture(VkImage texture, RenderSys::TextureDescriptor textureDesc, const void* textureData);
        void SubmitCommandBuffer();
        uint32_t GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        VkImage m_ImageToRenderInto = VK_NULL_HANDLE;
        VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
        VkImageView m_imageViewToRenderInto = VK_NULL_HANDLE;
        VkImage m_depthimageToRenderInto = VK_NULL_HANDLE;
        VkDeviceMemory m_depthimageMemory = VK_NULL_HANDLE;
        VkImageView m_depthimageViewToRenderInto = VK_NULL_HANDLE;

        VkSampler m_textureSampler;
        VkDescriptorSet m_descriptorSet;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
        std::unordered_map<std::string, std::vector<uint32_t>> m_shaderMap;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
        VkRenderPass m_renderpass = VK_NULL_HANDLE;

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
        std::unordered_map<uint32_t, std::tuple<VkDescriptorBufferInfo, VmaAllocation, void*>> m_uniformBuffers; // tuple -> <VkDescriptorBufferInfo, uniformBufferMemory, mappedBuffer>

        VmaAllocator m_vma = VK_NULL_HANDLE;
    };
}