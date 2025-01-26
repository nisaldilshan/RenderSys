#pragma once

#include <vector>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include "../Buffer.h"
#include "../RenderUtil.h"

namespace GraphicsAPI
{
    class VulkanCompute
    {
    public:
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateShaders(const char *shaderSource);
        void CreatePipeline();
        void CreateBuffer(uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type, const std::string& name);
        void SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string& name);
        void BeginComputePass();
        void Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
        void EndComputePass();
        std::vector<uint8_t>& GetMappedResult();
    };
}