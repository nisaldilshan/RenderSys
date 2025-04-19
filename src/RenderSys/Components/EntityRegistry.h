#pragma once

#include <entt/entt.hpp>

namespace RenderSys
{

class EntityRegistry
{
public:
    EntityRegistry() = default;
    static entt::registry& Get();
    ~EntityRegistry() = default;
    EntityRegistry(const EntityRegistry&) = delete;
    EntityRegistry& operator=(const EntityRegistry&) = delete;
    EntityRegistry(EntityRegistry&&) = delete;
    EntityRegistry& operator=(EntityRegistry&&) = delete;
private:
    
};
    
} // namespace RenderSys
    
