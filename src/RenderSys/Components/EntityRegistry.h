#pragma once

#include <entt/entt.hpp>

namespace RenderSys
{

namespace EntityRegistry
{

entt::registry& Get();  
void Destroy(); 

} // namespace EntityRegistry


    
} // namespace RenderSys
    
