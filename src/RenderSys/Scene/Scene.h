#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <RenderSys/Scene/SceneGraph.h>
#include <RenderSys/Scene/UUID.h>

namespace RenderSys 
{

class ICamera;

class Scene
{
public:
	Scene();
	~Scene();
	Scene(Scene const& other) = delete;
	Scene& operator=(Scene const& other) = delete;
	Scene(Scene&& other) noexcept = delete;
	Scene& operator=(Scene&& other) noexcept = delete;

	entt::entity CreateEntity(const std::string& name);
	entt::entity CreateEntityWithUUID(UUID uuid, const std::string& name);
	void DestroyEntity(entt::entity entity);
	void DestroyAllEntities();
	void Update();

	SceneGraph::TreeNode& GetSceneGraphTreeNode(uint32_t nodeIndex);
	void printNodeGraph() const;
	void AddInstanceOfSubTree(const uint32_t instanceIndex, const glm::vec3& pos, const uint32_t subTreeNodeIndex, uint32_t parent);
	void AddMeshInstanceOfEntity(const uint32_t instanceIndex, entt::entity& entity, const glm::vec3& translation, const uint32_t parentNodeIndex);

	void AddDirectionalLight(const glm::vec3 &direction, const glm::vec3 &position, const glm::vec3 &color);
	entt::entity AddCamera(std::shared_ptr<RenderSys::ICamera> camera);

	entt::registry m_Registry;
	SceneGraph m_sceneGraph;
	uint32_t m_rootNodeIndex = 0;
	uint32_t m_instancedRootNodeIndex = 1;

private:
	void UpdateTransformCache(uint32_t const nodeIndex, glm::mat4 const& parentMat4, bool parentDirtyFlag);
};

} // namespace RenderSys