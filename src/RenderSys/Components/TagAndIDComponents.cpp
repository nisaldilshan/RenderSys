#include "TagAndIDComponents.h"
#include <resources/Shaders/ShaderResource.h>

namespace RenderSys
{

void InstanceTagComponent::AddInstance(entt::entity instanceEntity)
{
    assert(m_instances.size() < MAX_INSTANCE);
    m_instances.emplace_back(instanceEntity);
}

uint32_t InstanceTagComponent::GetInstanceCount() const
{
    return static_cast<uint32_t>(m_instances.size());
}

std::shared_ptr<InstanceBuffer> InstanceTagComponent::GetInstanceBuffer() const
{
    return m_instanceBuffer;
}

} // namespace RenderSys