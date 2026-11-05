#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <imgui.h>
#include <RenderSys/Renderer3D.h>

// A "GPU timing" checkbox, then the renderer's GPU time per frame and per pass. The numbers are
// averaged over half a second, so they are readable while they change. Call it once per UI frame
// inside a window, e.g. the examples' Settings window under "Last render".
class GpuTimingPanel
{
public:
    void OnImGuiRender(RenderSys::Renderer3D& renderer)
    {
        if (!renderer.IsGpuTimingSupported())
        {
            ImGui::TextDisabled("GPU timing: not supported on this device");
            return;
        }

        bool enabled = renderer.IsGpuTimingEnabled();
        if (ImGui::Checkbox("GPU timing", &enabled))
        {
            renderer.SetGpuTimingEnabled(enabled);
            reset();
        }
        if (!enabled)
            return;

        accumulate(renderer.GetGpuTimings());
        if (m_shown.empty())
        {
            ImGui::TextDisabled("  measuring...");
            return;
        }
        for (const auto& [name, milliseconds] : m_shown)
            ImGui::Text("  %s: %.3f ms", name.c_str(), milliseconds);
    }

private:
    static constexpr double kAveragePeriodSeconds = 0.5;

    void reset()
    {
        m_sums.clear();
        m_shown.clear();
        m_frames = 0;
        m_windowStart = ImGui::GetTime();
    }

    // Adds a new timed frame to the running sums, and shows their average every half second
    void accumulate(const RenderSys::GpuFrameTimings& timings)
    {
        if (timings.valid && timings.frameIndex != m_lastFrameIndex)
        {
            m_lastFrameIndex = timings.frameIndex;
            add("GPU frame", timings.frameMilliseconds);
            for (const auto& pass : timings.passes)
                add(pass.name, pass.milliseconds);
            m_frames++;
        }

        const double now = ImGui::GetTime();
        if (now - m_windowStart < kAveragePeriodSeconds || m_frames == 0)
            return;
        m_shown = m_sums;
        for (auto& entry : m_shown)
            entry.second /= static_cast<float>(m_frames);
        m_sums.clear();
        m_frames = 0;
        m_windowStart = now;
    }

    void add(const std::string& name, float milliseconds)
    {
        for (auto& entry : m_sums)
        {
            if (entry.first == name)
            {
                entry.second += milliseconds;
                return;
            }
        }
        m_sums.emplace_back(name, milliseconds);
    }

    // Name and milliseconds, in the order the renderer reports them, with the whole frame first
    std::vector<std::pair<std::string, float>> m_sums;
    std::vector<std::pair<std::string, float>> m_shown;
    uint32_t m_frames = 0;
    uint64_t m_lastFrameIndex = 0;
    double m_windowStart = 0.0;
};
