#include "Compute.h"

#include "WebGPU/WebGPUCompute.h"

Compute::Compute()
    : m_computeBackend(std::make_unique<GraphicsAPI::WebGPUCompute>())
{
}

Compute::~Compute()
{
}

void Compute::SetShader(const char *shaderSource)
{
    m_computeBackend->CreateShaders(shaderSource);
}

void Compute::CreateBindGroup(const std::vector<wgpu::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    m_computeBackend->CreateBindGroup(bindGroupLayoutEntries);
}

void Compute::CreatePipeline()
{
    m_computeBackend->CreatePipeline();
}

void Compute::CreateBuffer(const uint32_t bufferLength, ComputeBuf::BufferType type, const std::string& name)
{
    m_computeBackend->CreateBuffer(bufferLength, type, name);
}

void Compute::SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string &name)
{
    m_computeBackend->SetBufferData(bufferData, bufferLength, name);
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

std::vector<uint8_t> &Compute::GetMappedResult()
{
    return m_computeBackend->GetMappedResult();
}
