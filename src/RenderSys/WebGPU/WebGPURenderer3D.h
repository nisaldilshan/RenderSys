#pragma once

#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>
#include <RenderSys/Buffer.h>
#include <RenderSys/Texture.h>

namespace GraphicsAPI
{
    struct WebGPUVertexIndexBufferInfo
    {
        wgpu::Buffer m_vertexBuffer = nullptr;
        uint32_t m_vertexCount = 0;
        wgpu::Buffer m_indexBuffer = nullptr;
        uint32_t m_indexCount = 0;
        //VkVertexInputBindingDescription m_vertextBindingDescs;
        //std::vector<VkVertexInputAttributeDescription> m_vertextAttribDescs;
    };

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
        void CreatePipeline();
        void CreateFrameBuffer();
        uint32_t CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData);
        void SetClearColor(glm::vec4 clearColor);
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform);
        // Textures get created as a part of main bindgroup
        void CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture);
        void CreateModelMaterials(uint32_t modelID, const std::vector<RenderSys::Material> &materials 
            , const std::vector<std::shared_ptr<RenderSys::Texture>>& textures, const int maxNumOfModels);
        void SetUniformData(uint32_t binding, const void* bufferData);
        void BindResources();
        void Render();
        void RenderIndexed();
        void RenderMesh(const RenderSys::Mesh& mesh);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void DestroyImages();
        void DestroyPipeline();
        void DestroyBindGroup();
        void Destroy();
    private:
        void CreateDefaultTextureSampler();
        void SubmitCommandBuffer();
        uint32_t GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        wgpu::Color m_clearColor = wgpu::Color{ 0.9, 0.1, 0.2, 1.0 };

        wgpu::ShaderModule m_shaderModule = nullptr;
        wgpu::RenderPipeline m_pipeline = nullptr;
        wgpu::TextureView m_textureToRenderInto = nullptr;

        wgpu::VertexBufferLayout m_vertexBufferLayout;
        std::unordered_map<uint32_t, WebGPUVertexIndexBufferInfo> m_vertexIndexBufferInfoMap;

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

        std::unordered_map<uint32_t, std::shared_ptr<RenderSys::WebGPUTexture>> m_textures;
        wgpu::Sampler m_defaultTextureSampler = nullptr;

        uint32_t m_width, m_height;
    };
}