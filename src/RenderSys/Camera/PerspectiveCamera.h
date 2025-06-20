#pragma once
#include <RenderSys/Components/CameraComponents.h>

#include "ICamera.h"

namespace RenderSys
{

class PerspectiveCamera : public ICamera
{
public:
    PerspectiveCamera(float fov, float nearClip, float farClip);
    ~PerspectiveCamera() override = default;

    void SetAspectRatio(float aspectRatio);

    glm::vec3 GetUpDirection() const;
    glm::vec3 GetRightDirection() const;
    glm::vec3 GetForwardDirection() const;

    void SetPosition(const glm::vec3& position);
    void SetOrientation(const glm::vec3& orientation);
private:
    void UpdateProjection();
    void UpdateView();

    float m_FOV = 0.0f;
    float m_AspectRatio = 1.0f;
    float m_nearClip = 0.0f;
    float m_farClip = 0.0f;
};


} // namespace Camera
