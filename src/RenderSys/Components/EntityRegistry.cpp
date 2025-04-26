#include "EntityRegistry.h"
#include <iostream>

namespace RenderSys
{

entt::registry g_registry;
entt::registry &RenderSys::EntityRegistry::Get()
{
    return g_registry;
}

void EntityRegistry::Destroy()
{
    auto allEntities = g_registry.view<entt::entity>(); // Create a view of all entities
    std::cout << "Destroying all entities." << std::endl;
    for (auto entity : allEntities) {
        //std::cout << "Destroying Entity [ID: " << int(entity) << "]" << std::endl;
        g_registry.destroy(entity);
    }
}

} // namespace RenderSys