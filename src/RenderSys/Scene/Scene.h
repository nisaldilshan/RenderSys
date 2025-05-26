#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <RenderSys/Scene/SceneGraph.h>
#include <RenderSys/Scene/UUID.h>

namespace RenderSys 
{

class Scene
{
public:
	Scene();
	~Scene();

	entt::entity CreateEntity(const std::string& name);
	entt::entity CreateEntityWithUUID(UUID uuid, const std::string& name);
	void DestroyEntity(entt::entity entity);
	void DestroyAllEntities();
	void UpdateTransformCache(uint32_t const nodeIndex, glm::mat4 const& parentMat4, bool parentDirtyFlag);

	SceneGraph::TreeNode& GetSceneGraphTreeNode(uint32_t nodeIndex);
	void printNodeGraph() const;

	entt::registry m_Registry;
	SceneGraph m_sceneGraph;
	uint32_t m_rootNodeIndex = 0;
	uint32_t m_instancedrootNodeIndex = 1;
};

} // namespace RenderSys