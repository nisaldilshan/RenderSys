#pragma once

#include <string>
#include <entt/entt.hpp>
#include <RenderSys/Scene/UUID.h>
#include <RenderSys/InstanceBuffer.h>
#include <RenderSys/Components/TransformComponent.h>

namespace RenderSys
{

class IDComponent
{
public:
    UUID ID;

    IDComponent() = delete;
    IDComponent(UUID& uuid)
        : ID(uuid) 
    {}
    IDComponent(const IDComponent&) = default;
};

class TagComponent
{
public:
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag) {}
};

class InstanceTagComponent
{
public:
    InstanceTagComponent() = delete;

    // Shares a buffer owned by other entities too (e.g. copies of one mesh, each drawing its own slot).
    explicit InstanceTagComponent(std::shared_ptr<InstanceBuffer> sharedInstanceBuffer)
        : m_instances()
        , m_instanceBuffer(std::move(sharedInstanceBuffer))
    {
        assert(m_instanceBuffer);
    }

    void AddInstance(entt::entity instanceEntity);
    uint32_t GetInstanceCount() const;
    std::shared_ptr<InstanceBuffer> GetInstanceBuffer() const;

private:
    std::vector<entt::entity> m_instances;
    std::shared_ptr<InstanceBuffer> m_instanceBuffer;
};
   
} // namespace RenderSys
