#include "VulkanCompute.h"

namespace GraphicsAPI
{

VulkanCompute::~VulkanCompute()
{
}

void VulkanCompute::Init()
{
}

void VulkanCompute::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry> &bindGroupLayoutEntries)
{
}

void VulkanCompute::CreateShaders(RenderSys::Shader &shader)
{
}

void VulkanCompute::CreatePipeline()
{
}

void VulkanCompute::CreateBuffer(uint32_t binding, uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type)
{
}

void VulkanCompute::SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength)
{
}

void VulkanCompute::BeginComputePass()
{
}

void VulkanCompute::Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY)
{
}

void VulkanCompute::EndComputePass()
{
}

std::vector<uint8_t>& VulkanCompute::GetMappedResult(uint32_t binding)
{
    auto& found = m_shaderOutputBuffers.find(binding);
    assert(found != m_shaderOutputBuffers.end());
    auto& mappedBufferStruct = found->second;
    
    while (!mappedBufferStruct->resultReady.load())
    {

    }

    return mappedBufferStruct->mappedData;
}

void VulkanCompute::Destroy()
{
}

}

