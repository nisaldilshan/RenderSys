#pragma once
#include "Skeleton.h"

namespace RenderSys
{

class SkeletalAnimation
{

public:
    enum class Path
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    enum class InterpolationMethod
    {
        LINEAR,
        STEP,
        CUBICSPLINE
    };

    struct Channel
    {
        Path m_Path;
        int m_SamplerIndex;
        int m_Node;
    };

    struct Sampler
    {
        std::vector<float> m_Timestamps;
        std::vector<glm::vec4> m_TRSoutputValuesToBeInterpolated;
        InterpolationMethod m_Interpolation;
    };

public:
    SkeletalAnimation(std::string const &name);

    void Start();
    void Stop();
    bool IsRunning() const;
    bool WillExpire(const float &timestep) const;
    std::string const &GetName() const { return m_Name; }
    void SetRepeat(bool repeat) { m_Repeat = repeat; }
    void Update(const float &timestep, Skeleton &skeleton);
    float GetDuration() const { return m_LastKeyFrameTime - m_FirstKeyFrameTime; }
    float GetCurrentTime() const { return m_CurrentKeyFrameTime - m_FirstKeyFrameTime; }

    std::vector<SkeletalAnimation::Sampler> m_Samplers;
    std::vector<SkeletalAnimation::Channel> m_Channels;

    void SetFirstKeyFrameTime(float firstKeyFrameTime) { m_FirstKeyFrameTime = firstKeyFrameTime; }
    void SetLastKeyFrameTime(float lastKeyFrameTime) { m_LastKeyFrameTime = lastKeyFrameTime; }

private:
    std::string m_Name;
    bool m_Repeat;

    // relative animation time
    float m_FirstKeyFrameTime;
    float m_LastKeyFrameTime;
    float m_CurrentKeyFrameTime = 0.0f;
};

} // namespace RenderSys
