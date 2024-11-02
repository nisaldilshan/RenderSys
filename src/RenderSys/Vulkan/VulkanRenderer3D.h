#pragma once

#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include "../RenderUtil.h"
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
        void CreateShaders(const char* shaderSource);
        void CreateStandaloneShader(const char *shaderSource, uint32_t vertexShaderCallCount);
        void CreatePipeline();
        void CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(const std::vector<uint16_t> &bufferData);
        void SetClearColor(glm::vec4 clearColor);
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateUniformBuffer(size_t bufferLength, UniformBuf::UniformType type, uint32_t sizeOfUniform, uint32_t bindingIndex);
        void CreateDepthTexture();
        void CreateTexture(uint32_t textureWidth, uint32_t textureHeight, const void* textureData, uint32_t mipMapLevelCount);
        void CreateTextureSampler();
        void SetUniformData(UniformBuf::UniformType type, const void* bufferData, uint32_t uniformIndex);
        void SimpleRender();
        void Render(uint32_t uniformIndex);
        void RenderIndexed(uint32_t uniformIndex);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void Reset();
        
    private:
        void UploadTexture(VkImage texture, RenderSys::TextureDescriptor textureDesc, const void* textureData);
        void SubmitCommandBuffer();
        uint32_t GetOffset(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        // wgpu::Color m_clearColor = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

        // wgpu::ShaderModule m_shaderModule = nullptr;
        // wgpu::RenderPipeline m_pipeline = nullptr;

        VkImage m_image;
        VkImageView m_imageView;
        VkDeviceMemory m_imageMemory;
        VkSampler m_imageSampler;
        VkDescriptorSet m_descriptorSet; // same as m_textureToRenderInto

        uint32_t m_vertexCount = 0;
        uint64_t m_vertexBufferSize = 0;
        // wgpu::Buffer m_vertexBuffer = nullptr;
        // wgpu::VertexBufferLayout m_vertexBufferLayout;

        uint32_t m_indexCount = 0;
        // wgpu::Buffer m_indexBuffer = nullptr;

        // wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
        // wgpu::PipelineLayout m_pipelineLayout = nullptr;

        // std::unordered_map<UniformBuf::UniformType, std::tuple<uint32_t, wgpu::Buffer, uint32_t>> m_uniformBuffers;
        // wgpu::BindGroup m_bindGroup = nullptr;

        // wgpu::CommandEncoder m_currentCommandEncoder = nullptr;
        // wgpu::RenderPassEncoder m_renderPass = nullptr;

        // wgpu::TextureFormat m_depthTextureFormat =  wgpu::TextureFormat::Undefined;
        // wgpu::Texture m_depthTexture = nullptr;
        // wgpu::TextureView m_depthTextureView = nullptr;

        // std::vector<std::pair<wgpu::Texture, wgpu::TextureView>> m_texturesAndViews;
        // wgpu::Sampler m_textureSampler = nullptr;

        uint32_t m_width, m_height;
    };
}