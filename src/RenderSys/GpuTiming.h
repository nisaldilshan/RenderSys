#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace RenderSys
{

// GPU time of one pass, such as "Shadow" or "Main"
struct GpuPassTime
{
    std::string name;
    float milliseconds = 0.0f;
};

// GPU time of the last timed frame, from timestamp queries the renderer writes around the frame
// and around each pass. Renderer3D::GetGpuTimings returns it.
struct GpuFrameTimings
{
    // False while timing is off or unsupported, and before the first timed frame has finished
    bool valid = false;
    // Counts timed frames, so a caller polling every UI frame can tell a new result from a repeat
    uint64_t frameIndex = 0;
    // From BeginFrame to EndFrame, including any gaps between the passes
    float frameMilliseconds = 0.0f;
    // In recording order
    std::vector<GpuPassTime> passes;
};

} // namespace RenderSys
