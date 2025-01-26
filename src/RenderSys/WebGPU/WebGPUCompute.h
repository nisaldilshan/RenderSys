#pragma once

#include <stdint.h>
#include <stddef.h>

#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include "../Buffer.h"
#include "../RenderUtil.h"

namespace GraphicsAPI
{
    class WebGPUCompute
    {
    public:
        WebGPUCompute() = default;
        ~WebGPUCompute();

        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateShaders(const char *shaderSource);
        void CreatePipeline();
        void CreateBuffer(uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type, const std::string& name);
        void SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string& name);
        void BeginComputePass();
        void Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
        void EndComputePass();
        std::vector<uint8_t>& GetMappedResult();
    private:
        wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
        wgpu::BindGroup m_bindGroup = nullptr;
        wgpu::ShaderModule m_shaderModule = nullptr;
        wgpu::ComputePipeline m_pipeline = nullptr;
        wgpu::CommandEncoder m_commandEncoder = nullptr;
        wgpu::ComputePassEncoder m_computePass = nullptr;

        std::unordered_map<std::string, wgpu::Buffer> m_buffersAccessibleToShader;
        
    };
}