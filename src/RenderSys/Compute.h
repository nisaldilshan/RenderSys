#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include "Buffer.h"
#include "RenderUtil.h"
#include "Shader.h"

namespace GraphicsAPI
{
#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
class VulkanCompute;
typedef VulkanCompute ComputeType;
#elif (RENDERER_BACKEND == 3)
class WebGPUCompute;
typedef WebGPUCompute ComputeType;
#else
static_assert(false);
#endif
}

namespace RenderSys
{
    
class Compute
{
public:
    Compute();
    ~Compute();

    Compute(const Compute&) = delete;
	Compute &operator=(const Compute&) = delete;
	Compute(Compute&&) = delete;
	Compute &operator=(Compute&&) = delete;

    void Init();
    void SetShader(RenderSys::Shader& shader);
    void CreatePipeline();
    void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreateBuffer(uint32_t binding, const uint32_t bufferLength, ComputeBuf::BufferType type);
    void SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength);
    void BeginComputePass();
    void DoCompute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
    void EndComputePass();
    std::vector<uint8_t>& GetMappedResult(const uint32_t binding);
    void Destroy();
private:
    std::unique_ptr<GraphicsAPI::ComputeType> m_computeBackend;
};

} // namespace RenderSys



