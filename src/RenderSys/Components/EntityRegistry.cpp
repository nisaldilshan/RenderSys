#include "EntityRegistry.h"
#include <iostream>

namespace RenderSys
{

entt::registry g_registry;
entt::registry &RenderSys::EntityRegistry::Get()
{
    // auto allEntities = g_registry.view<entt::entity>(); // Create a view of all entities

    // // You can also use a range-based for loop with the view
    // for (auto entity : allEntities) {
    //     std::cout << "Entity ID (via view range-based for): " << int(entity) << std::endl;
    // }

    return g_registry;
}


} // namespace RenderSys