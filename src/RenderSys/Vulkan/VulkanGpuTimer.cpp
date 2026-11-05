#include "VulkanGpuTimer.h"

#include <RenderSys/Vulkan/VulkanContext.h>

#include <array>
#include <iostream>

namespace RenderSys
{
namespace Vulkan
{

GpuTimer::~GpuTimer()
{
    Destroy();
}

void GpuTimer::Init()
{
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(GetPhysicalDevice(), &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(GetPhysicalDevice(), &familyCount, families.data());

    const uint32_t family = GetQueueFamily();
    const uint32_t validBits = family < familyCount ? families[family].timestampValidBits : 0;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(GetPhysicalDevice(), &properties);

    m_supported = validBits > 0 && properties.limits.timestampPeriod > 0.0f;
    m_nanosecondsPerTick = properties.limits.timestampPeriod;
    m_timestampMask = validBits >= 64 ? ~0ull : ((1ull << validBits) - 1ull);
    if (!m_supported)
        std::cout << "GPU timing: this device's queue cannot write timestamps, so timing stays off" << std::endl;
}

void GpuTimer::Destroy()
{
    if (m_queryPool)
    {
        vkDestroyQueryPool(GetDevice(), m_queryPool, nullptr);
        m_queryPool = VK_NULL_HANDLE;
    }
    m_frameActive = false;
    m_passOpen = false;
    m_timings = {};
}

bool GpuTimer::CreateQueryPool()
{
    if (m_queryPool)
        return true;

    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = kMaxQueries;
    if (vkCreateQueryPool(GetDevice(), &createInfo, nullptr, &m_queryPool) != VK_SUCCESS)
    {
        std::cout << "GPU timing: vkCreateQueryPool failed, so timing stays off" << std::endl;
        m_queryPool = VK_NULL_HANDLE;
        m_supported = false;
        return false;
    }
    return true;
}

void GpuTimer::BeginFrame(VkCommandBuffer commandBuffer)
{
    m_frameActive = false;
    m_passOpen = false;
    if (!IsEnabled() || !CreateQueryPool())
    {
        m_timings.valid = false;
        m_timings.passes.clear();
        return;
    }

    // Outside any render pass, as vkCmdResetQueryPool requires
    vkCmdResetQueryPool(commandBuffer, m_queryPool, 0, kMaxQueries);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_queryPool, 0);
    m_queryCount = 1;
    m_passes.clear();
    m_frameActive = true;
}

void GpuTimer::BeginPass(VkCommandBuffer commandBuffer, const char* name)
{
    // Keep one query free for the frame's end
    if (!m_frameActive || m_passOpen || m_queryCount + 2 > kMaxQueries - 1)
        return;

    // BOTTOM_OF_PIPE: written once the work recorded before it has finished, so back-to-back passes
    // split the frame between them instead of overlapping
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_queryCount);
    m_passes.push_back({name, m_queryCount});
    m_queryCount++;
    m_passOpen = true;
}

void GpuTimer::EndPass(VkCommandBuffer commandBuffer)
{
    if (!m_frameActive || !m_passOpen)
        return;

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_queryCount);
    m_queryCount++;
    m_passOpen = false;
}

void GpuTimer::EndFrame(VkCommandBuffer commandBuffer)
{
    if (!m_frameActive)
        return;

    if (m_passOpen)
        EndPass(commandBuffer);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_queryCount);
    m_queryCount++;
}

void GpuTimer::ReadResults()
{
    if (!m_frameActive)
        return;
    m_frameActive = false;

    std::array<uint64_t, kMaxQueries> ticks{};
    const VkResult result = vkGetQueryPoolResults(GetDevice(), m_queryPool, 0, m_queryCount,
                                                  sizeof(uint64_t) * m_queryCount, ticks.data(), sizeof(uint64_t),
                                                  VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (result != VK_SUCCESS)
    {
        m_timings.valid = false;
        return;
    }

    auto milliseconds = [&](uint32_t beginQuery, uint32_t endQuery) {
        const uint64_t elapsed = ((ticks[endQuery] & m_timestampMask) - (ticks[beginQuery] & m_timestampMask)) & m_timestampMask;
        return static_cast<float>(static_cast<double>(elapsed) * m_nanosecondsPerTick * 1e-6);
    };

    m_timings.frameMilliseconds = milliseconds(0, m_queryCount - 1);
    m_timings.passes.clear();
    for (const Pass& pass : m_passes)
        m_timings.passes.push_back({pass.name, milliseconds(pass.beginQuery, pass.beginQuery + 1)});
    m_timings.valid = true;
    m_timings.frameIndex++;
}

} // namespace Vulkan
} // namespace RenderSys
