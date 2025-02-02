#pragma once

#include <stdint.h>
#include <stddef.h>
#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include <RenderSys/Buffer.h>
#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>

struct MappedBuffer 
{
    wgpu::Buffer buffer = nullptr;;
    wgpu::Buffer mapBuffer = nullptr;
    std::atomic<bool> resultReady = false;
    std::vector<uint8_t> mappedData;
};

namespace GraphicsAPI
{
    class WebGPUCompute
    {
    public:
        WebGPUCompute() = default;
        ~WebGPUCompute();

        void Init();
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateShaders(RenderSys::Shader& shader);
        void CreatePipeline();
        void CreateBuffer(uint32_t binding, uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type);
        void SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength);
        void BeginComputePass();
        void Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
        void BufferMapCallback(WGPUMapAsyncStatus status, char const * message, uint32_t binding);
        void EndComputePass();
        std::vector<uint8_t>& GetMappedResult(uint32_t binding);
        void Destroy();
    private:
        wgpu::BindGroupLayout m_bindGroupLayout = nullptr;
        wgpu::BindGroup m_bindGroup = nullptr;
        wgpu::ShaderModule m_shaderModule = nullptr;
        wgpu::ComputePipeline m_pipeline = nullptr;
        wgpu::CommandEncoder m_commandEncoder = nullptr;
        wgpu::ComputePassEncoder m_computePass = nullptr;

        std::unordered_map<uint32_t, wgpu::Buffer> m_buffersAccessibleToShader;
        std::unordered_map<uint32_t, std::shared_ptr<MappedBuffer>> m_shaderOutputBuffers;
    };
}