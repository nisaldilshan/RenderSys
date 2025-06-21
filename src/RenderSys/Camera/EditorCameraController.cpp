#include "EditorCameraController.h"

#include <Walnut/Input/Input.h>

namespace RenderSys
{

EditorCameraController::EditorCameraController(float fov, float nearClip, float farClip)
    : m_Camera(std::make_shared<PerspectiveCamera>(fov, nearClip, farClip))
{
    m_Camera->SetAspectRatio(1.0f); // Default aspect ratio, can be updated later
}

void EditorCameraController::OnUpdate()
{
    if (Walnut::Input::IsKeyDown(Walnut::Key::LeftAlt))
    {
        const glm::vec2& mouse = Walnut::Input::GetMousePosition();
        static glm::vec2 initialMousePosition{ 0.0f, 0.0f };
        glm::vec2 delta = (mouse - initialMousePosition) * 0.003f;
        initialMousePosition = mouse;

        if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Middle))
            MousePan(delta);
        else if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Left))
            MouseRotate(delta);
        else if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Right))
            MouseZoom(delta.y);
    }

    m_Camera->SetOrientation(glm::vec3(m_Pitch, m_Yaw, 0.0f));
    m_Camera->SetPosition(m_FocalPoint - m_Camera->GetForwardDirection() * m_Distance);
}

void EditorCameraController::MousePan(const glm::vec2 &delta)
{
    m_FocalPoint += -m_Camera->GetRightDirection() * delta.x * PanSpeed();
    m_FocalPoint += m_Camera->GetUpDirection() * delta.y * PanSpeed();
}

void EditorCameraController::MouseRotate(const glm::vec2 &delta)
{
    float yawSign = m_Camera->GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * RotationSpeed();
    m_Pitch += delta.y * RotationSpeed();
}

void EditorCameraController::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
}

float EditorCameraController::RotationSpeed() const
{
    return 1.0f;
}

float EditorCameraController::ZoomSpeed() const
{
    return 5.0f;
}

float EditorCameraController::PanSpeed() const
{
    return 0.5f * m_Distance; // Scale pan speed based on distance from focal point
}

} // namespace RenderSys