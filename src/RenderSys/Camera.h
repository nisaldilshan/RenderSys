#pragma once
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Camera
{
    

class PerspectiveCamera
{
public:
    PerspectiveCamera(float fov, float nearClip, float farClip);
    ~PerspectiveCamera() = default;

    void SetViewportSize(float width, float height);
    void OnUpdate();
    glm::vec3 GetPosition() const;
    glm::mat4x4 GetViewMatrix();
    glm::mat4x4 GetProjectionMatrix() const;
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

    glm::mat4x4 m_view = glm::mat4(1.0f);
    glm::mat4x4 m_projection = glm::mat4(1.0f);

    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
    float m_Distance = 2.0f;

    float m_FOV = 0.0f;
    float m_nearClip = 0.0f;
    float m_farClip = 0.0f;

    float m_Pitch = 0.0f; // this was -2.0
    float m_Yaw = 0.0f;

    float m_viewportWidth = 0;
    float m_viewportHeight = 0;
};


} // namespace Camera
