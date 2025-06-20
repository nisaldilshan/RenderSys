#pragma once
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <memory>

namespace RenderSys
{

class EditorCameraController
{
public:
    EditorCameraController();
    ~EditorCameraController() = default;

    void OnUpdate();

    std::shared_ptr<RenderSys::PerspectiveCamera> GetCamera() { return m_Camera; }
private:
    void MousePan(const glm::vec2& delta);
    void MouseRotate(const glm::vec2& delta);
    void MouseZoom(float delta);

    float RotationSpeed() const;
    float ZoomSpeed() const;
    float PanSpeed() const;

    std::shared_ptr<RenderSys::PerspectiveCamera> m_Camera;
    glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
    float m_Distance = 2.0f;

    float m_Pitch = 0.0f; // this was -2.0
    float m_Yaw = 0.0f;
};

} // namespace RenderSys