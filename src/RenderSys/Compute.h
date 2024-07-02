#pragma once

#include <memory>
#include <stdint.h>

#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include "Buffer.h"

namespace GraphicsAPI
{
#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
static_assert(false);
#elif (RENDERER_BACKEND == 3)
class WebGPUCompute;
typedef WebGPUCompute ComputeType;
#else
#endif
}

namespace RenderSys
{
    
class Compute
{
public:
    Compute();
    ~Compute();
    void SetShader(const char* shaderSource);
    void CreateBindGroup(const std::vector<wgpu::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreatePipeline();
    void CreateBuffer(const uint32_t bufferLength, ComputeBuf::BufferType type, const std::string& name);
    void SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string& name);
    void BeginComputePass();
    void DoCompute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
    void EndComputePass();
    std::vector<uint8_t>& GetMappedResult();
private:
    std::unique_ptr<GraphicsAPI::ComputeType> m_computeBackend;
};

} // namespace RenderSys



