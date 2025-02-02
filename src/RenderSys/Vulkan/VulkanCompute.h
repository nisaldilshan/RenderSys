#pragma once

#include <vector>
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include <RenderSys/Buffer.h>
#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>

namespace GraphicsAPI
{
    class VulkanCompute
    {
    public:
        VulkanCompute() = default;
        ~VulkanCompute();
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateShaders(RenderSys::Shader& shader);
        void CreatePipeline();
        void CreateBuffer(uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type, const std::string& name);
        void SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string& name);
        void BeginComputePass();
        void Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
        void EndComputePass();
        std::vector<uint8_t>& GetMappedResult();
    private:
        std::vector<uint8_t> m_mapBufferMappedData;
        std::atomic<bool> m_resultReady = false;
    };
}