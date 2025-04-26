#include "TransformComponent.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>

namespace RenderSys
{

TransformComponent::TransformComponent()
    : m_Scale(glm::vec3(1.0)), m_Rotation(glm::vec3(0.0)), m_Translation(glm::vec3(0.0)), m_Dirty(true)
{
    //std::cout << "  # TransformComponent Created: " << this << std::endl;
}

void TransformComponent::SetMat4Local(const glm::mat4 &mat4)
{
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(mat4, scale, rotation, translation, skew, perspective);
    glm::vec3 rotationEuler = glm::eulerAngles(rotation);

    SetTranslation(translation);
    SetRotation(rotationEuler);
    SetScale(scale);
}

void TransformComponent::SetMat4Global(const glm::mat4 &parent)
{
    if (m_InstanceBuffer)
    {
        // auto mat4Global = parent * GetMat4Local();
        // m_InstanceBuffer->SetInstanceData(m_InstanceIndex, mat4Global);
    }
    else
    {
        m_Mat4Global = parent * GetMat4Local();
    }
    m_Parent = parent;
}

const glm::mat4 &TransformComponent::GetMat4Local()
{
    if (m_Dirty)
    {
        RecalculateMatrices();
    }
    return m_Mat4Local;
}

const glm::mat4 &TransformComponent::GetMat4Global()
{
    // if (m_InstanceBuffer)
    // {
    //     return m_InstanceBuffer->GetModelMatrix(m_InstanceIndex);
    // }
    // else
    {
        return m_Mat4Global;
    }
}

void TransformComponent::RecalculateMatrices()
{
    auto scale = glm::scale(glm::mat4(1.0f), m_Scale);
    auto rotation = glm::toMat4(glm::quat(m_Rotation));
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3{m_Translation.x, m_Translation.y, m_Translation.z});

    m_Mat4Local = translation * rotation * scale;

    m_Dirty = false;
}

void TransformComponent::SetScale(const glm::vec3 &scale)
{
    m_Scale = scale;
    m_Dirty = true;
}

void TransformComponent::SetRotation(const glm::vec3& rotation)
{
    m_Rotation = rotation;
    m_Dirty = true;
}

void TransformComponent::SetRotation(const glm::quat &quaternion)
{
    glm::vec3 convert = glm::eulerAngles(quaternion);
    // ZYX - model in Blender
    SetRotation(glm::vec3{convert.x, convert.y, convert.z});
}

void TransformComponent::SetTranslation(const glm::vec3 &translation)
{
    m_Translation = translation;
    m_Dirty = true;
}

} // namespace RenderSys