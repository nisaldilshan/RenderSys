#include "EditorCameraController.h"

#include <Walnut/Input/Input.h>

namespace RenderSys
{

EditorCameraController::EditorCameraController(float fov, float nearClip, float farClip)
    : m_Camera(std::make_shared<PerspectiveCamera>(fov, nearClip, farClip))
{
    m_Camera->SetAspectRatio(1.0f); // Default aspect ratio, can be updated later
    m_Camera->SetOrientation(glm::vec3(0.0f, 0.0f, 0.0f));

    glm::vec3 initialFocalPoint = { 0.0f, 0.0f, 0.0f };
    float initialDistance = 2.0f;
    m_Camera->SetPosition(initialFocalPoint - m_Camera->GetForwardDirection() * initialDistance);
}

void EditorCameraController::OnUpdate()
{
    if (Walnut::Input::IsKeyDown(Walnut::Key::LeftAlt))
    {
        const glm::vec2& mouse = Walnut::Input::GetMousePosition();
        if (m_prevMousePosition == glm::vec2(0.0f, 0.0f))
        {
            m_prevMousePosition = mouse;
            return;
        }
        glm::vec2 delta = (mouse - m_prevMousePosition) * 0.003f;
        m_prevMousePosition = mouse;

        float yaw = m_Camera->GetRotation().y;
        float pitch = m_Camera->GetRotation().x;
        glm::vec3 pos = m_Camera->GetPosition();
        if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Middle))
        {
            MousePan(delta, pos);
        }
        else if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Left))
        {
            MouseRotate(delta, yaw, pitch);
        }
        else if (Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Right))
        {
            MouseZoom(delta.y, pos);
        }

        m_Camera->SetOrientation(glm::vec3(pitch, yaw, 0.0f));
        m_Camera->SetPosition(pos);
    }
    else
    {
        m_prevMousePosition = { 0.0f, 0.0f };
    }
}

void EditorCameraController::OnUpdate(TransformComponent &transformComponent)
{
    float yaw = transformComponent.GetRotation().y;
    const glm::vec3 forwardDir{std::sin(yaw), 0.f, std::cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.f, -forwardDir.x};
    const glm::vec3 upDir{0.f, -1.f, 0.f};

    m_Camera->SetPosition(transformComponent.GetTranslation());
    m_Camera->SetOrientation(transformComponent.GetRotation());
}

void EditorCameraController::MousePan(const glm::vec2 &delta, glm::vec3 &position)
{
    position += -m_Camera->GetRightDirection() * delta.x * PanSpeed();
    position += m_Camera->GetUpDirection() * delta.y * PanSpeed();
}

void EditorCameraController::MouseRotate(const glm::vec2 &delta, float& yaw, float& pitch)
{
    float yawSign = m_Camera->GetUpDirection().y < 0 ? -1.0f : 1.0f;
    yaw += yawSign * delta.x * RotationSpeed();
    pitch += delta.y * RotationSpeed();
}

void EditorCameraController::MouseZoom(float delta, glm::vec3 &position)
{
    position += m_Camera->GetForwardDirection() * delta * ZoomSpeed();
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
    return 10.0f;
}

} // namespace RenderSys