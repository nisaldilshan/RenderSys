#pragma once

#include <memory>
#include <stdint.h>
#include <stddef.h>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include "../RenderUtil.h"
#include "../Shader.h"

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
        void CreateShaders(RenderSys::Shader& shader);
        void CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
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
        void SubmitCommandBuffer();
        uint32_t GetOffset(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform);

        bool m_inited = false;
        VkShaderModule m_shaderModuleVertex = 0;
        VkShaderModule m_shaderModuleFragment = 0;
        uint32_t m_vertexCount = 0;
        uint64_t m_vertexBufferSize = 0;
        std::unique_ptr<VkPipelineLayoutCreateInfo> m_bindGroupLayout;
        uint32_t m_width, m_height;
    };
}