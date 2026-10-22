#pragma once

#include "ICamera.h"

namespace RenderSys
{

// Left is what RenderSys always used, and it shows right-handed data such as glTF mirrored.
// Right draws it as authored; pair it with FrontFace::CounterClockwise in Renderer3D::CreatePipeline.
// Either way the camera looks along +Z of its rotation, so a position and orientation see the same place.
enum class Handedness
{
    Left = 0,
    Right,
};

class PerspectiveCamera : public ICamera
{
public:
    PerspectiveCamera(float fov, float nearClip, float farClip, Handedness handedness = Handedness::Left);
    ~PerspectiveCamera() override = default;

    Handedness GetHandedness() const { return m_handedness; }

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;

    void SetPosition(const glm::vec3& position);
    void SetOrientation(const glm::vec3& orientation);

    float GetFOV() const { return m_FOV; }
    float GetAspectRatio() const { return m_AspectRatio; }
    float GetNearClip() const { return m_nearClip; }
    float GetFarClip() const { return m_farClip; }

    void SetFOV(float fov) { m_FOV = fov; UpdateProjection(); }
    void SetNearClip(float nearClip) { m_nearClip = nearClip; UpdateProjection(); }
    void SetFarClip(float farClip) { m_farClip = farClip; UpdateProjection (); }
    void SetAspectRatio(float aspectRatio);

private:
    void UpdateProjection();
    void UpdateView();

    float m_FOV = 0.0f;
    float m_AspectRatio = 1.0f;
    float m_nearClip = 0.0f;
    float m_farClip = 0.0f;
    Handedness m_handedness = Handedness::Left;
};


} // namespace Camera
