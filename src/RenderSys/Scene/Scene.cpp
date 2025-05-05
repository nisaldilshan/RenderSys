#include "Scene.h"
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/TransformComponent.h>

namespace RenderSys
{

Scene::Scene() {}

Scene::~Scene() {}

entt::entity Scene::CreateEntity(const std::string& name)
{
	return CreateEntityWithUUID(UUID(), name);
}

entt::entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
	entt::entity entity{m_Registry.create()};
	m_Registry.emplace<IDComponent>(entity, uuid);
	m_Registry.emplace<TransformComponent>(entity);
	auto& tag = m_Registry.emplace<TagComponent>(entity);
	tag.Tag = name.empty() ? "Entity" : name;
	return entity;
}

void Scene::DestroyEntity(entt::entity entity)
{
	m_Registry.destroy(entity);
}

} // namespace Hazel