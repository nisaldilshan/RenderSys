#pragma once

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
    PerspectiveCameraComponent(float aspectRatio, float yfov, float zfar, float znear)
        : m_AspectRatio(aspectRatio), m_YFov(yfov), m_ZFar(zfar), m_ZNear(znear)
    {
    }
    PerspectiveCameraComponent() = delete;
    float m_AspectRatio;
    float m_YFov;
    float m_ZFar;
    float m_ZNear;
};

}