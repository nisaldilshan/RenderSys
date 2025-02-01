#include "VulkanCompute.h"

namespace GraphicsAPI
{

VulkanCompute::~VulkanCompute()
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

void VulkanCompute::CreateBuffer(uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type, const std::string &name)
{
}

void VulkanCompute::SetBufferData(const void *bufferData, uint32_t bufferLength, const std::string &name)
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

std::vector<uint8_t> &VulkanCompute::GetMappedResult()
{
    if (!m_resultReady)
        assert(false);

    return m_mapBufferMappedData;
}

}

