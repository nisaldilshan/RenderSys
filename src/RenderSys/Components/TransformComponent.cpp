#include "TransformComponent.h"

#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>
#include <RenderSys/InstanceBuffer.h>

namespace RenderSys
{

TransformComponent::TransformComponent()
    : m_Scale(glm::vec3(1.0)), m_Rotation(glm::vec3(0.0)), m_Translation(glm::vec3(0.0)), m_Dirty(true)
{}

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
        const auto mat4Global = parent * GetMat4Local();
        m_InstanceBuffer->SetInstanceData(m_InstanceIndex, mat4Global);
    }
    else
    {
        m_Mat4Global = parent * GetMat4Local();
    }
    m_Parent = parent;
}

void TransformComponent::SetMat4Global()
{
    if (m_InstanceBuffer)
    {
        auto mat4Global = GetMat4Local();
        m_InstanceBuffer->SetInstanceData(m_InstanceIndex, mat4Global);
    }
    else
    {
        m_Mat4Global = GetMat4Local();
    }
}

const glm::mat4 &TransformComponent::GetMat4Local()
{
    if (m_Dirty)
    {
        RecalculateMatrices();
    }
    return m_Mat4Local;
}

const glm::mat4 &TransformComponent::GetMat4Global() const
{
    if (m_InstanceBuffer)
    {
        return m_InstanceBuffer->GetModelMatrix(m_InstanceIndex);
    }
    else
    {
        return m_Mat4Global;
    }
}

bool TransformComponent::GetDirtyFlag() const
{
    return m_Dirty;
}

void TransformComponent::SetInstance(std::shared_ptr<RenderSys::InstanceBuffer> instanceBuffer, uint32_t instanceIndex)
{
    m_InstanceBuffer = instanceBuffer;
    m_InstanceIndex = instanceIndex;
}

void TransformComponent::RecalculateMatrices()
{
    auto scale = glm::scale(glm::mat4(1.0f), m_Scale);
    auto rotation = glm::toMat4(glm::quat(m_Rotation));
    auto translation = glm::translate(glm::mat4(1.0f), glm::vec3{m_Translation.x, m_Translation.y, m_Translation.z});

    m_Mat4Local = translation * rotation * scale;

    m_Dirty = false;

    if (m_changeNotifyCallback)
    {
        std::invoke(m_changeNotifyCallback, m_Translation, m_Rotation);
    }
}

void TransformComponent::SetScale(const glm::vec3 &scale)
{
    if (m_Scale == scale)
    {
        return; // No change, no need to recalculate
    }
    m_Scale = scale;
    m_Dirty = true;
}

void TransformComponent::SetRotation(const glm::vec3& rotation)
{
    if (m_Rotation == rotation)
    {
        return; // No change, no need to recalculate
    }
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
    if (m_Translation == translation)
    {
        return; // No change, no need to recalculate
    }
    m_Translation = translation;
    m_Dirty = true;
}

const glm::mat4& TransformComponent::GetParent() const { return m_Parent; }

} // namespace RenderSys