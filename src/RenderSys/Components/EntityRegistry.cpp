#include "EntityRegistry.h"

namespace RenderSys
{

entt::registry &RenderSys::EntityRegistry::Get()
{
    static entt::registry registry;
    return registry;
}


} // namespace RenderSys