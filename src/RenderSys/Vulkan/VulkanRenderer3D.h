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

    struct VulkanUniformBufferInfo
    {
        VkDescriptorBufferInfo m_bufferInfo = {VK_NULL_HANDLE, 0, 0};
        VmaAllocation m_uniformBufferMemory = VK_NULL_HANDLE;
        void* m_mappedBuffer = nullptr;
    };

    // material
#define GLSL_HAS_DIFFUSE_MAP (0x1 << 0x0)
#define GLSL_HAS_NORMAL_MAP (0x1 << 0x1)
#define GLSL_HAS_ROUGHNESS_MAP (0x1 << 0x2)
#define GLSL_HAS_METALLIC_MAP (0x1 << 0x3)
#define GLSL_HAS_ROUGHNESS_METALLIC_MAP (0x1 << 0x4)
#define GLSL_HAS_EMISSIVE_COLOR (0x1 << 0x5)
#define GLSL_HAS_EMISSIVE_MAP (0x1 << 0x6)

#define GLSL_NUM_MULTI_MATERIAL 4

    enum MaterialFeatures // bitset
    {
        HAS_DIFFUSE_MAP = GLSL_HAS_DIFFUSE_MAP,
        HAS_NORMAL_MAP = GLSL_HAS_NORMAL_MAP,
        HAS_ROUGHNESS_MAP = GLSL_HAS_ROUGHNESS_MAP,
        HAS_METALLIC_MAP = GLSL_HAS_METALLIC_MAP,
        HAS_ROUGHNESS_METALLIC_MAP = GLSL_HAS_ROUGHNESS_METALLIC_MAP,
        HAS_EMISSIVE_COLOR = GLSL_HAS_EMISSIVE_COLOR,
        HAS_EMISSIVE_MAP = GLSL_HAS_EMISSIVE_MAP
    };

    struct MaterialProperties
    { // align data to blocks of 16 bytes
        // byte 0 to 15
        uint32_t m_features{0};
        float m_roughness{0.0f};
        float m_metallic{0.0f};
        float m_NormalMapIntensity{1.0f};

        // byte 16 to 31
        glm::vec4 m_baseColor{1.0f, 1.0f, 1.0f, 1.0f};

        // byte 32 to 47
        glm::vec3 m_EmissiveColor{0.0f, 0.0f, 0.0f};
        float m_EmissiveStrength{1.0f};
    };

    struct VulkanMaterial
    {
        VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; // 1 bind group for different texture types of one material (baseColor/normal/metallic-roughness)
        MaterialProperties m_materialProperties;
    };

    struct VulkanModelInfo
    {
        std::vector<VulkanMaterial> m_materials;
    };

    class VulkanRenderer3D
    {
    public:
        VulkanRenderer3D() = default;
        ~VulkanRenderer3D() = default;

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
        void CreateModelMaterials(uint32_t modelID, const std::vector<RenderSys::Material>& materials
            , const std::vector<std::shared_ptr<RenderSys::Texture>>& textures, const int maxNumOfModels);
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
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

        std::unordered_map<uint32_t, VulkanVertexIndexBufferInfo> m_vertexIndexBufferInfoMap;

        VkDescriptorPool m_bindGroupPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_bindGroupLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_mainBindGroup = VK_NULL_HANDLE;

        VkDescriptorPool m_materialBindGroupPool = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_materialBindGroupLayout = VK_NULL_HANDLE;

        // map modelID to VulkanModelInfo
        std::unordered_map<uint32_t, VulkanModelInfo> m_models;

        std::unordered_map<uint32_t, std::shared_ptr<RenderSys::VulkanTexture>> m_textures;
        // map bindingNumber to tuple -> <VkDescriptorBufferInfo, uniformBufferMemory, mappedBuffer>
        std::unordered_map<uint32_t, std::tuple<VkDescriptorBufferInfo, VmaAllocation, void*>> m_uniformBuffers;
        VkClearColorValue m_clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    };
}