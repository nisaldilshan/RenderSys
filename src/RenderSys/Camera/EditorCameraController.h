#pragma once
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <RenderSys/Components/TransformComponent.h>

#include <memory>

namespace RenderSys
{

class EditorCameraController
{
public:
    EditorCameraController() = delete;
    EditorCameraController(float fov, float nearClip, float farClip);
    ~EditorCameraController() = default;

    void OnUpdate();
    void OnUpdate(TransformComponent& transformComponent);

    std::shared_ptr<RenderSys::PerspectiveCamera> GetCamera() { return m_Camera; }
private:
    void MousePan(const glm::vec2 &delta, glm::vec3 &position);
    void MouseRotate(const glm::vec2 &delta, float& yaw, float& pitch);
    void MouseZoom(float delta, glm::vec3 &position);

    float RotationSpeed() const;
    float ZoomSpeed() const;
    float PanSpeed() const;

    std::shared_ptr<RenderSys::PerspectiveCamera> m_Camera;
    glm::vec2 m_prevMousePosition = { 0.0f, 0.0f };
};

} // namespace RenderSys