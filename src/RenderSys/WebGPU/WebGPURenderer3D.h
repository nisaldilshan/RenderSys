#pragma once

#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include "../RenderUtil.h"
#include "../Shader.h"
#include "../Buffer.h"

namespace GraphicsAPI
{
    class WebGPURenderer3D
    {
    public:
        WebGPURenderer3D() = default;
        ~WebGPURenderer3D() = default;

        bool Init();
        void CreateImageToRender(uint32_t width, uint32_t height);
        void CreateDepthImage();
        void CreateTextureSamplers(const std::vector<RenderSys::TextureSampler>& samplers);
        void CreateShaders(RenderSys::Shader& shader);
        void CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
        void CreatePipeline();
        void CreateFrameBuffer();
        void CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(const std::vector<uint32_t> &bufferData);
        void SetClearColor(glm::vec4 clearColor);
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform, uint32_t uniformCountInBuffer);
        // Textures get created as a part of main bindgroup
        void CreateTexture(uint32_t binding, const RenderSys::TextureDescriptor& texDescriptor);
        // Textures get created in separate bindgroup
        void CreateTextures(const std::vector<RenderSys::TextureDescriptor>& texDescriptors);
        void CreateMaterialBindGroups(const std::vector<RenderSys::Material>& materials);
        void SetUniformData(uint32_t binding, const void* bufferData, uint32_t uniformIndex);
        void BindResources();
        void Render(uint32_t uniformIndex);
        void RenderIndexed(uint32_t uniformIndex);
        void RenderMesh(const RenderSys::Mesh& mesh, uint32_t uniformIndex);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void DestroyImages();
        void DestroyPipeline();
        void DestroyBindGroup();
        void Destroy();
    private:
        void CreateDefaultTextureSampler();
        void UploadTexture(wgpu::Texture texture, wgpu::TextureDescriptor textureDesc, const void* textureData);
        void SubmitCommandBuffer();
        uint32_t GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        wgpu::Color m_clearColor = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

        wgpu::ShaderModule m_shaderModule = nullptr;
        wgpu::RenderPipeline m_pipeline = nullptr;
        wgpu::TextureView m_textureToRenderInto = nullptr;

        uint32_t m_vertexCount = 0;
        uint64_t m_vertexBufferSize = 0;
        wgpu::Buffer m_vertexBuffer = nullptr;
        wgpu::VertexBufferLayout m_vertexBufferLayout;

        uint32_t m_indexCount = 0;
        wgpu::Buffer m_indexBuffer = nullptr;

        wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
        std::vector<wgpu::BindGroupLayoutEntry> m_mainBindGroupBindings;
        wgpu::PipelineLayout m_pipelineLayout = nullptr;

        // map bindingNumber to tuple -> <VkDescriptorBufferInfo, uniformBufferMemory, mappedBuffer>
        std::unordered_map<uint32_t, std::tuple<wgpu::Buffer, uint32_t>> m_uniformBuffers;
        wgpu::BindGroup m_bindGroup = nullptr;

        wgpu::CommandEncoder m_currentCommandEncoder = nullptr;
        wgpu::RenderPassEncoder m_renderPass = nullptr;

        wgpu::Texture m_depthTexture = nullptr;
        wgpu::TextureView m_depthTextureView = nullptr;

        std::vector<std::pair<wgpu::Texture, wgpu::TextureView>> m_texturesAndViews;
        wgpu::Sampler m_defaultTextureSampler = nullptr;

        uint32_t m_width, m_height;
    };
}