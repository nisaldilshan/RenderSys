#pragma once

#include <stdint.h>
#include <stddef.h>

#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include "../RenderUtil.h"
#include "../Shader.h"

namespace GraphicsAPI
{
    class WebGPURenderer2D
    {
    public:
        WebGPURenderer2D() = default;
        ~WebGPURenderer2D() = default;

        void Init();
        void CreateTextureToRenderInto(uint32_t width, uint32_t height);
        void CreateShaders(RenderSys::Shader& shader);
        void CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
        void CreatePipeline();
        void CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(const std::vector<uint16_t> &bufferData);
        void SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry);
        void CreateBindGroup();
        void CreateUniformBuffer(size_t uniformCountInBuffer, uint32_t sizeOfOneUniform);
        void SetUniformData(const void* bufferData, uint32_t uniformIndex);
        void SimpleRender();
        void Render();
        void RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        void Destroy();
        
    private:
        void SubmitCommandBuffer();
        uint32_t GetOffset(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

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
        wgpu::PipelineLayout m_pipelineLayout = nullptr;
        wgpu::Buffer m_uniformBuffer = nullptr;
        wgpu::BindGroup m_bindGroup = nullptr;
        uint32_t m_sizeOfUniform = 0;

        wgpu::CommandEncoder m_currentCommandEncoder = nullptr;
        wgpu::RenderPassEncoder m_renderPass = nullptr;

        uint32_t m_width, m_height;
    };
}