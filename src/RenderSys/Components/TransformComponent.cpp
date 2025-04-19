#include "TransformComponent.h"

namespace RenderSys
{
void TransformComponent::SetMat4Local(const glm::mat4 &mat4)
{
}

void TransformComponent::SetMat4Global(const glm::mat4 &parent)
{
}

const glm::mat4 &TransformComponent::GetMat4Global()
{
    return m_Mat4Global;
}

TransformComponent::TransformComponent()
{
}

void TransformComponent::SetScale(const glm::vec3 &scale)
{
}

void TransformComponent::SetRotation(const glm::quat &quaternion)
{
}

void TransformComponent::SetTranslation(const glm::vec3 &translation)
{
}

} // namespace RenderSys