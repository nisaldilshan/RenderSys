#include "PerspectiveCamera.h"

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

void PerspectiveCamera::UpdateProjection()
{
    m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_nearClip, m_farClip);
}

void PerspectiveCamera::SetOrientation(const glm::vec3& orientation)
{
    m_Rotation = orientation;
    UpdateView();
}

void PerspectiveCamera::SetPosition(const glm::vec3& position)
{
    m_Position = position;
    UpdateView();
}

void PerspectiveCamera::UpdateView()
{
    //m_ViewMatrix = glm::lookAt(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0, 0, 1));
    glm::quat orientation = glm::quat(m_Rotation);
    m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
    m_ViewMatrix = glm::inverse(m_ViewMatrix);
}

glm::vec3 PerspectiveCamera::GetUpDirection() const
{
    return glm::rotate(glm::quat(m_Rotation), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 PerspectiveCamera::GetRightDirection() const
{
    return glm::rotate(glm::quat(m_Rotation), glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 PerspectiveCamera::GetForwardDirection() const
{
    return glm::rotate(glm::quat(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
}
    
} // namespace Camera


