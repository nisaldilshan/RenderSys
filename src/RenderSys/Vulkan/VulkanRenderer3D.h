#pragma once

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

namespace GraphicsAPI
{
    struct VulkanVertexIndexBufferInfo
    {
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VmaAllocation m_vertexBufferMemory = VK_NULL_HANDLE;
        uint32_t m_vertexCount = 0;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VmaAllocation m_indexBufferMemory = VK_NULL_HANDLE;
        uint32_t m_indexCount = 0;
        VkVertexInputBindingDescription m_vertextBindingDescs;
        std::vector<VkVertexInputAttributeDescription> m_vertextAttribDescs;
    };

    struct VulkanMaterial
    {
        VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
        int materialUniformBufferSlot = -1;
        int baseColorTextureIndex = -1;
        int normalTextureIndex = -1;
        int metallicRoughnessTextureIndex = -1;
    };

    struct VulkanModelInfo
    {
        std::vector<VulkanMaterial> m_materials;
        std::vector<std::tuple<VkImage, VmaAllocation, VkDescriptorImageInfo>> m_sceneTextures;
        std::vector<VkSampler> m_sceneTextureSamplers;
    };

    class VulkanRenderer3D
    {
    public:
        VulkanRenderer3D() = default;
        ~VulkanRenderer3D() = default;

        bool Init();
        void CreateImageToRender(uint32_t width, uint32_t height);
        void CreateDepthImage();
        void CreateTextureSamplers(const std::vector<RenderSys::TextureSampler>& samplers);
        void CreateShaders(RenderSys::Shader& shader);
        void CreatePipeline();
        void CreateFrameBuffer();
        uint32_t CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData);
        void SetClearColor(glm::vec4 clearColor);
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform, uint32_t uniformCountInBuffer);
        // Textures get created as a part of main bindgroup
        void CreateTexture(uint32_t binding, const RenderSys::TextureDescriptor& texDescriptor);
        // Textures get created in separate bindgroup
        void CreateTextures(const std::vector<RenderSys::TextureDescriptor>& texDescriptors);
        void CreateMaterialBindGroups(uint32_t modelID, const std::vector<RenderSys::Material>& materials);
        void SetUniformData(uint32_t binding, const void* bufferData);
        void BindResources();
        void Render();
        void RenderIndexed();
        void RenderMesh(const RenderSys::Mesh& mesh);
        void RenderPrimitive(const uint32_t vertexBufferID, const uint32_t indexCount, const uint32_t firstIndex, const VulkanMaterial &material);
        void DrawPlane();
        void DrawCube();
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void DestroyImages();
        void DestroyPipeline();
        void DestroyBindGroup();
        void Destroy();
    private:
        void CreateDefaultTextureSampler();
        void CreatePipelineLayout();
        void CreateRenderPass();
        void CreateCommandBuffers();
        void DestroyRenderPass();
        void DestroyBuffers();
        void DestroyShaders();
        void DestroyTextures();
        void UploadTexture(VkImage texture, const RenderSys::TextureDescriptor& texDescriptor);
        void SubmitCommandBuffer();

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        VkImage m_ImageToRenderInto = VK_NULL_HANDLE;
        VmaAllocation m_renderImageMemory = VK_NULL_HANDLE;
        VkImageView m_imageViewToRenderInto = VK_NULL_HANDLE;
        VkImage m_depthimage = VK_NULL_HANDLE;
        VmaAllocation m_depthimageMemory = VK_NULL_HANDLE;
        VkImageView m_depthimageView = VK_NULL_HANDLE;

        VkSampler m_defaultTextureSampler = VK_NULL_HANDLE;
        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
        std::unordered_map<std::string, std::vector<uint32_t>> m_shaderMap;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
        VkRenderPass m_renderpass = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

        std::unordered_map<uint32_t, VulkanVertexIndexBufferInfo> m_vertexIndexBufferInfoMap;

        VkDescriptorPool m_bindGroupPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_bindGroupLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_mainBindGroup = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_materialBindGroupLayout = VK_NULL_HANDLE;

        // map modelID to VulkanModelInfo
        std::unordered_map<uint32_t, VulkanModelInfo> m_models;

        // map bindingNumber to tuple -> <image, textureMemory, VkDescriptorImageInfo>
        std::unordered_map<uint32_t, std::tuple<VkImage, VmaAllocation, VkDescriptorImageInfo>> m_textures;
        std::vector<std::tuple<VkImage, VmaAllocation, VkDescriptorImageInfo>> m_sceneTextures;
        std::vector<VkSampler> m_sceneTextureSamplers;

        // map bindingNumber to tuple -> <VkDescriptorBufferInfo, uniformBufferMemory, mappedBuffer>
        std::unordered_map<uint32_t, std::tuple<VkDescriptorBufferInfo, VmaAllocation, void*>> m_uniformBuffers;

        VmaAllocator m_vma = VK_NULL_HANDLE;
        VkClearColorValue m_clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };
}