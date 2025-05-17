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
    InstanceTagComponent()
        : m_instances() 
        , m_instanceBuffer(std::make_shared<InstanceBuffer>())
    {}

    void AddInstance(entt::entity instanceEntity);
    uint32_t GetInstanceCount() const;
    std::shared_ptr<InstanceBuffer> GetInstanceBuffer() const;

private:
    std::vector<entt::entity> m_instances;
    std::shared_ptr<InstanceBuffer> m_instanceBuffer;
};
   
} // namespace RenderSys
