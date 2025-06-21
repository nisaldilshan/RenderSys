#pragma once

#include <RenderSys/Camera/ICamera.h>

namespace RenderSys
{

struct PointLightComponent
{
    float m_LightIntensity{1.0f};
    float m_Radius{1.0f};
    glm::vec3 m_Color{1.0f, 1.0f, 1.0f};
};

struct DirectionalLightComponent
{
    float m_LightIntensity{1.0f};
    glm::vec3 m_Color{1.0f, 1.0f, 1.0f};
    glm::vec3 m_Direction{};
    ICamera* m_LightView{nullptr};
    int m_RenderPass{0};
};

}