#pragma once

#include <entt/entt.hpp>
#include "UUID.h"

namespace RenderSys 
{

class Scene
{
public:
	Scene();
	~Scene();

	entt::entity CreateEntity(const std::string& name = std::string());
	entt::entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
	void DestroyEntity(entt::entity entity);

	entt::registry m_Registry;
};

} // namespace RenderSys