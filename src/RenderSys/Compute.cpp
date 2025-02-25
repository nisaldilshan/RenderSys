#include "Compute.h"

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanCompute.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPUCompute.h"
#else
static_assert(false);
#endif

using namespace RenderSys;

Compute::Compute()
    : m_computeBackend(std::make_unique<GraphicsAPI::ComputeType>())
{}

Compute::~Compute()
{}

void Compute::Init()
{
    m_computeBackend->Init();
}

void Compute::SetShader(RenderSys::Shader& shader)
{
    m_computeBackend->CreateShaders(shader);
}

void Compute::CreatePipeline()
{
    m_computeBackend->CreatePipeline();
}

void Compute::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    m_computeBackend->CreateBindGroup(bindGroupLayoutEntries);
}

void Compute::CreateBuffer(uint32_t binding, const uint32_t bufferLength, ComputeBuf::BufferType type)
{
    m_computeBackend->CreateBuffer(binding, bufferLength, type);
}

void Compute::SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength)
{
    m_computeBackend->SetBufferData(binding, bufferData, bufferLength);
}

void Compute::BeginComputePass()
{
    m_computeBackend->BeginComputePass();
}

void Compute::DoCompute(const uint32_t workgroupCountX, const uint32_t workgroupCountY)
{
    m_computeBackend->Compute(workgroupCountX, workgroupCountY);
}

void Compute::EndComputePass()
{
    m_computeBackend->EndComputePass();
}

std::vector<uint8_t> &Compute::GetMappedResult(const uint32_t binding)
{
    return m_computeBackend->GetMappedResult(binding);
}

void Compute::Destroy()
{
    m_computeBackend->Destroy();
}
