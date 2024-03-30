#pragma once

#include <memory>
#include <stdint.h>

#include <Walnut/GraphicsAPI/WebGPUGraphics.h>

#include "Buffer.h"

#define RENDERER_BACKEND 3

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

class Compute
{
public:
    Compute();
    ~Compute();
    void SetShader(const char* shaderSource);
    void CreateBindGroup(const std::vector<wgpu::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreatePipeline();
    void CreateBuffer(const uint32_t bufferLength, ComputeBuf::BufferType type);
    void BeginComputePass();
    void DoCompute(const void* bufferData, const uint32_t bufferLength);
    void EndComputePass();
private:
    std::unique_ptr<GraphicsAPI::ComputeType> m_computeBackend;
};

