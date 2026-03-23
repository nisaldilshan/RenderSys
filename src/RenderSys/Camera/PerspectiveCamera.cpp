#include "PerspectiveCamera.h"
#include <Walnut/RenderingBackend.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


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
    m_ProjectionMatrix = glm::perspectiveLH_ZO(glm::radians(m_FOV), m_AspectRatio, m_nearClip, m_farClip);
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
    const glm::vec3 target = m_Position + GetForwardDirection();
    m_ViewMatrix = glm::lookAtLH(m_Position, target, GetUpDirection());
}

glm::vec3 PerspectiveCamera::GetUpDirection() const
{
    //return glm::rotate(glm::quat(m_Rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat3 rotationMatrix = glm::mat3_cast(glm::quat(m_Rotation));
    return rotationMatrix[1]; // Second column
}

glm::vec3 PerspectiveCamera::GetRightDirection() const
{
    //return glm::rotate(glm::quat(m_Rotation), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat3 rotationMatrix = glm::mat3_cast(glm::quat(m_Rotation));
    return rotationMatrix[0]; // First column
}

glm::vec3 PerspectiveCamera::GetForwardDirection() const
{
    //return glm::rotate(glm::quat(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat3 rotationMatrix = glm::mat3_cast(glm::quat(m_Rotation));
    return rotationMatrix[2]; // Third column
}
    
} // namespace Camera


