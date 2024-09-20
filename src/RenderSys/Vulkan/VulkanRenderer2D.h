#pragma once

#include <memory>
#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include "../RenderUtil.h"


class VkPipelineLayoutCreateInfo;

namespace GraphicsAPI
{
    struct RenderSysVkVertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };

    struct RenderSysVkMesh {
        std::vector<RenderSysVkVertex> vertices;
    };
    class VulkanRenderer2D
    {
    public:
        VulkanRenderer2D() = default;
        ~VulkanRenderer2D() = default;

        bool Init();
        void CreateTextureToRenderInto(uint32_t width, uint32_t height);
        void CreateShaders(const char* shaderSource);
        void CreateStandaloneShader(const char *shaderSource, uint32_t vertexShaderCallCount);
        void CreatePipeline();
        void CreateFrameBuffer();
        void CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
        void CreateIndexBuffer(const std::vector<uint16_t> &bufferData);
        void SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry);
        void CreateBindGroup();
        void CreateUniformBuffer(size_t bufferLength, uint32_t sizeOfUniform);
        void SetUniformData(const void* bufferData, uint32_t uniformIndex);
        void SimpleRender();
        void Render();
        void RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount);
        ImTextureID GetDescriptorSet();
        void BeginRenderPass();
        void EndRenderPass();
        
    private:
        bool CreateSwapChain();
        void SubmitCommandBuffer();
        uint32_t GetOffset(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        VkShaderModule m_shaderModuleVertex = 0;
        VkShaderModule m_shaderModuleFragment = 0;
        // wgpu::RenderPipeline m_pipeline = nullptr;
        // wgpu::TextureView m_textureToRenderInto = nullptr;

        uint32_t m_vertexCount = 0;
        uint64_t m_vertexBufferSize = 0;
        // wgpu::Buffer m_vertexBuffer = nullptr;
        // wgpu::VertexBufferLayout m_vertexBufferLayout;

        uint32_t m_indexCount = 0;
        // wgpu::Buffer m_indexBuffer = nullptr;

        std::unique_ptr<VkPipelineLayoutCreateInfo> m_bindGroupLayout;
        
        // wgpu::Buffer m_uniformBuffer = nullptr;
        // wgpu::BindGroup m_bindGroup = nullptr;
        uint32_t m_sizeOfUniform = 0;

        // wgpu::CommandEncoder m_currentCommandEncoder = nullptr;
        // wgpu::RenderPassEncoder m_renderPass = nullptr;

        uint32_t m_width, m_height;
    };
}