#pragma once
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <RenderSys/Components/TransformComponent.h>
#include <entt/entt.hpp>
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
    std::shared_ptr<RenderSys::PerspectiveCamera> GetCamera() { return m_Camera; }
    void SetCameraEntity(entt::entity cameraEntity, entt::registry& reg) { m_CameraEntity = cameraEntity; m_Registry = &reg; }
private:
    void MousePan(const glm::vec2 &delta, glm::vec3 &position);
    void MouseRotate(const glm::vec2 &delta, float& yaw, float& pitch);
    void MouseZoom(float delta, glm::vec3 &position);

    float RotationSpeed() const;
    float ZoomSpeed() const;
    float PanSpeed() const;

    std::shared_ptr<RenderSys::PerspectiveCamera> m_Camera;
    glm::vec2 m_prevMousePosition;
    entt::entity m_CameraEntity;
    entt::registry* m_Registry;
};

} // namespace RenderSys