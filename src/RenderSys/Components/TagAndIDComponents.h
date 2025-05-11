#pragma once

#include <string>
#include <RenderSys/Scene/UUID.h>
#include <RenderSys/Buffer.h>
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
    InstanceTagComponent() = default;

    void ResetInstanceBuffer(entt::registry& registry)
    {
        m_instanceBuffer = std::make_shared<Buffer>(sizeof(glm::mat4x4) * m_instances.size(), RenderSys::BufferUsage::STORAGE_BUFFER_VISIBLE_TO_CPU);
        m_instanceBuffer->MapBuffer();

        std::vector<glm::mat4x4> instanceData;
        for (const auto & instanceEntity : m_instances)
        {
            assert(registry.all_of<TransformComponent>(instanceEntity));
            auto& transform = registry.get<TransformComponent>(instanceEntity);
            instanceData.push_back(transform.GetMat4Local());
        }
        
        m_instanceBuffer->WriteToBuffer(instanceData.data());
    }

    std::vector<entt::entity> m_instances;
    std::shared_ptr<Buffer> m_instanceBuffer;
};
   
} // namespace RenderSys
