#include "PerspectiveCamera.h"
#include <Walnut/Input/Input.h>

namespace RenderSys
{

PerspectiveCamera::PerspectiveCamera(float fov, float nearClip, float farClip)
    : m_FOV(fov)
    , m_nearClip(nearClip)
    , m_farClip(farClip)
{}

void PerspectiveCamera::SetAspectRatio(float aspectRatio)
{
    m_AspectRatio = aspectRatio;
    UpdateProjection(); 
}

void PerspectiveCamera::OnUpdate()
{
    if (Walnut::Input::IsKeyDown(Walnut::Key::LeftAlt))
    {
        //std::cout << "Alt Down!!" << std::endl;
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
}

glm::vec3 PerspectiveCamera::GetPosition() const
{
    return m_Position; 
}

glm::mat4x4 PerspectiveCamera::GetViewMatrix() 
{
    UpdateView();
    return m_view; 
}

glm::mat4x4 PerspectiveCamera::GetProjectionMatrix() const 
{ 
    return m_projection; 
}

void PerspectiveCamera::UpdateProjection()
{
    m_projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_nearClip, m_farClip);
}

glm::quat PerspectiveCamera::GetOrientation() const
{
    return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
}

void PerspectiveCamera::UpdatePosition()
{
    m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
}

void PerspectiveCamera::UpdateView()
{
    UpdatePosition();

    //m_view = glm::lookAt(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0, 0, 1));
    glm::quat orientation = GetOrientation();
    m_view = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
    m_view = glm::inverse(m_view);
}

void PerspectiveCamera::MousePan(const glm::vec2 &delta)
{
    m_FocalPoint += -GetRightDirection() * delta.x * PanSpeed();
    m_FocalPoint += GetUpDirection() * delta.y * PanSpeed();
}

void PerspectiveCamera::MouseRotate(const glm::vec2 &delta)
{
    float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
    m_Yaw += yawSign * delta.x * RotationSpeed();
    m_Pitch += delta.y * RotationSpeed();
}

void PerspectiveCamera::MouseZoom(float delta)
{
    m_Distance -= delta * ZoomSpeed();
}

glm::vec3 PerspectiveCamera::GetUpDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 PerspectiveCamera::GetRightDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 PerspectiveCamera::GetForwardDirection() const
{
    return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, 1.0f));
}

float PerspectiveCamera::RotationSpeed() const
{
    return 1.0f;
}

float PerspectiveCamera::ZoomSpeed() const
{
    return 2.0f;
}

float PerspectiveCamera::PanSpeed() const
{
    return 0.5f * m_Distance; // Scale pan speed based on distance from focal point
}
    
} // namespace Camera


