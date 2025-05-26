#include "Scene.h"
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/TransformComponent.h>
#include <iostream>

namespace RenderSys
{

Scene::Scene() 
{
	m_rootNodeIndex = m_sceneGraph.CreateRootNode(CreateEntity("RootNode"), "RootNode");
}

Scene::~Scene() 
{
	
}

entt::entity Scene::CreateEntity(const std::string& name)
{
	return CreateEntityWithUUID(UUID(), name);
}

entt::entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
{
	entt::entity entity{m_Registry.create()};
	m_Registry.emplace<IDComponent>(entity, uuid);
	//m_Registry.emplace<TransformComponent>(entity);
	auto& tag = m_Registry.emplace<TagComponent>(entity);
	tag.Tag = name.empty() ? "Entity" : name;
	return entity;
}

void Scene::DestroyEntity(entt::entity entity)
{
	//std::cout << "Destroying Entity [ID: " << int(entity) << "]" << std::endl;
	m_Registry.destroy(entity);
}

void Scene::DestroyAllEntities()
{
	auto allEntities = m_Registry.view<entt::entity>(); // Create a view of all entities
	std::cout << "Destroying all entities." << std::endl;
	for (auto entity : allEntities) {
		DestroyEntity(entity);
	}
}

void Scene::UpdateTransformCache(uint32_t const nodeIndex, glm::mat4 const &parentMat4, bool parentDirtyFlag)
{
	auto& node = GetSceneGraphTreeNode(nodeIndex);
	entt::entity gameObject = node.GetGameObject();

	auto& transform = m_Registry.get<TransformComponent>(gameObject);
	bool dirtyFlag = transform.GetDirtyFlag() || parentDirtyFlag;

	if (dirtyFlag)
	{
		transform.SetMat4Global(parentMat4);
	}

	const glm::mat4& mat4Global = transform.GetMat4Global();
	for (uint32_t index = 0; index < node.Children(); index++)
	{
		UpdateTransformCache(node.GetChild(index), mat4Global, dirtyFlag);
	}
}

SceneGraph::TreeNode &Scene::GetSceneGraphTreeNode(uint32_t nodeIndex)
{
    return m_sceneGraph.GetNode(nodeIndex);
}

void Scene::printNodeGraph() const
{
	std::cout << "---- Scene begin ----\n";
    m_sceneGraph.TraverseLog(m_rootNodeIndex);
	std::cout << " -- Scene end --" << std::endl;
}

} // namespace Hazel