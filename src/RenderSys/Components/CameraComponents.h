#pragma once
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <memory>

namespace RenderSys
{

struct OrthographicCameraComponent
{
    OrthographicCameraComponent(float xmag, float ymag, float zfar, float znear)
        : m_XMag(xmag), m_YMag(ymag), m_ZFar(zfar), m_ZNear(znear)
    {
    }
    OrthographicCameraComponent() = delete;
    float m_XMag;
    float m_YMag;
    float m_ZFar;
    float m_ZNear;
};

struct PerspectiveCameraComponent
{
    PerspectiveCameraComponent() = default;
    ~PerspectiveCameraComponent() = default;
    PerspectiveCameraComponent(const PerspectiveCameraComponent&) = delete;
    PerspectiveCameraComponent &operator=(const PerspectiveCameraComponent&) = delete;
    PerspectiveCameraComponent(PerspectiveCameraComponent&&) = delete;
    PerspectiveCameraComponent &operator=(PerspectiveCameraComponent&&) = delete;

    std::shared_ptr<RenderSys::PerspectiveCamera> m_Camera;
    bool IsPrimary = false; // Indicates if this camera is the primary camera for the scene
};

}