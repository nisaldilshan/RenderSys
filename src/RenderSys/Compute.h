#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <imgui_impl_glfw.h>

#include "Buffer.h"
#include "RenderUtil.h"

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
    void SetShader(const char* shaderSource);
    void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
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



