#pragma once
#include <RenderSys/Components/CameraComponents.h>

#include "ICamera.h"

namespace RenderSys
{

class PerspectiveCamera : public ICamera
{
public:
    PerspectiveCamera(float fov, float nearClip, float farClip);
    ~PerspectiveCamera() override = default;

    void SetAspectRatio(float aspectRatio);
    void OnUpdate();
    glm::mat4x4 GetViewMatrix();
private:
    void UpdateProjection();
    glm::quat GetOrientation() const;
    void UpdatePosition();
    void UpdateView();

    void MousePan(const glm::vec2& delta);
    void MouseRotate(const glm::vec2& delta);
    void MouseZoom(float delta);

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;

    float RotationSpeed() const;
    float ZoomSpeed() const;
    float PanSpeed() const;

    glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
    float m_Distance = 2.0f;

    float m_FOV = 0.0f;
    float m_AspectRatio = 1.0f;
    float m_nearClip = 0.0f;
    float m_farClip = 0.0f;

    float m_Pitch = 0.0f; // this was -2.0
    float m_Yaw = 0.0f;
};


} // namespace Camera
