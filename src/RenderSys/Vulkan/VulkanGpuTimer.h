#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <vector>
#include <RenderSys/GpuTiming.h>

namespace RenderSys
{
namespace Vulkan
{

// Times a frame and its passes on the GPU with timestamp queries. The renderer calls BeginFrame
// after it starts recording the frame's command buffer, BeginPass and EndPass around each pass,
// EndFrame before it ends the command buffer, and ReadResults once the GPU has finished the frame.
// Every call is a no-op while timing is off, and the query pool is only created when it is first on.
//
// One query pool serves every frame, which is safe because the renderer waits for the GPU at the end
// of each frame (Vulkan::QueueSubmitAndWait). With several frames in flight it would need one pool
// per frame, read back when that frame's fence signals.
class GpuTimer
{
public:
    GpuTimer() = default;
    ~GpuTimer();

    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    // Checks whether the context's queue can write timestamps
    void Init();
    void Destroy();

    bool IsSupported() const { return m_supported; }
    // Takes effect at the next BeginFrame, so a frame is never half timed
    void SetEnabled(bool enabled) { m_enableRequested = enabled; }
    bool IsEnabled() const { return m_enableRequested && m_supported; }

    void BeginFrame(VkCommandBuffer commandBuffer);
    // Passes must not nest. Call outside a render pass, around vkCmdBeginRenderPass and vkCmdEndRenderPass.
    void BeginPass(VkCommandBuffer commandBuffer, const char* name);
    void EndPass(VkCommandBuffer commandBuffer);
    void EndFrame(VkCommandBuffer commandBuffer);
    // Call after the frame's submit has finished on the GPU
    void ReadResults();

    const GpuFrameTimings& GetTimings() const { return m_timings; }

private:
    // The frame's begin and end, plus a begin and end per pass: room for 31 passes
    static constexpr uint32_t kMaxQueries = 64;

    struct Pass
    {
        std::string name;
        uint32_t beginQuery = 0;
    };

    bool CreateQueryPool();

    bool m_supported = false;
    bool m_enableRequested = false;
    float m_nanosecondsPerTick = 1.0f;
    uint64_t m_timestampMask = ~0ull;
    VkQueryPool m_queryPool = VK_NULL_HANDLE;

    // State of the frame being recorded
    bool m_frameActive = false;
    bool m_passOpen = false;
    uint32_t m_queryCount = 0;
    std::vector<Pass> m_passes;

    GpuFrameTimings m_timings;
};

} // namespace Vulkan
} // namespace RenderSys
