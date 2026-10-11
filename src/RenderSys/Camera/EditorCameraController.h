#pragma once
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <RenderSys/Components/TransformComponent.h>
#include <entt/entt.hpp>
#include <memory>

namespace RenderSys
{

// Input state the controller reads each frame, filled by the host from its window system.
struct CameraInput
{
    glm::vec2 mousePosition{0.0f, 0.0f};
    bool navigate = false; // modifier that enables camera navigation, Left Alt in the examples
    bool leftButton = false;
    bool middleButton = false;
    bool rightButton = false;
};

class EditorCameraController
{
public:
    EditorCameraController() = delete;
    EditorCameraController(float fov, float nearClip, float farClip);
    ~EditorCameraController() = default;

    void OnUpdate(const CameraInput& input);
    std::shared_ptr<RenderSys::PerspectiveCamera> GetCamera() { return m_Camera; }
    void SetCameraEntity(entt::entity cameraEntity, entt::registry& reg);
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