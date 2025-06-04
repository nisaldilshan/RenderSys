#include "Scene.h"
#include <RenderSys/Resource.h>
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Components/TransformComponent.h>
#include <iostream>

namespace RenderSys
{

Scene::Scene() 
	: m_Registry()
	, m_sceneGraph()
	, m_rootNodeIndex(m_sceneGraph.CreateRootNode(CreateEntity("RootNode"), "RootNode"))
	, m_instancedRootNodeIndex(m_sceneGraph.CreateRootNode(CreateEntity("InstancedRootNode"), "InstancedRootNode"))
{
	std::cout << "Scene created with root node index: " << m_rootNodeIndex << std::endl;
	std::cout << "Instanced root node index: " << m_instancedRootNodeIndex << std::endl;
}

Scene::~Scene() 
{
	DestroyAllEntities();
}

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

void Scene::Update()
{
	// Update the transform cache for the root node
	UpdateTransformCache(m_instancedRootNodeIndex, glm::mat4(1.0f), false);
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

void Scene::AddInstanceofSubTree(const uint32_t instanceIndex, const glm::vec3& pos, const uint32_t subTreeNodeIndex, uint32_t parent)
{
	auto& childNode = m_sceneGraph.GetNode(subTreeNodeIndex);
	std::vector<uint32_t> children = childNode.GetChildren();
	if (children.size() == 0) 
	{
		return; // No children to process
	}
	
	for (auto childNodeIndex : children)
	{
		auto& childNode = m_sceneGraph.GetNode(childNodeIndex);
		auto nodeEntity = childNode.GetGameObject();
		assert(nodeEntity != entt::null);
		if (m_Registry.all_of<RenderSys::MeshComponent>(nodeEntity))
		{
			AddMeshInstanceOfEntity(instanceIndex, nodeEntity, pos, parent);
		}
		else
		{
			if (subTreeNodeIndex == m_rootNodeIndex)
			{
				// need a proper entity copy mechanism here.
				const auto name = childNode.GetName() + "_instance" + std::to_string(instanceIndex + 1);
				auto instanceModelTop = CreateEntity(name);
				parent = m_sceneGraph.CreateNode(m_instancedRootNodeIndex, instanceModelTop, name);
			}
			AddInstanceofSubTree(instanceIndex, pos, childNodeIndex, parent);
		}
	}
}

void Scene::AddMeshInstanceOfEntity(const uint32_t instanceIndex, entt::entity& entity, const glm::vec3& translation, const uint32_t parentNodeIndex)
{
	auto& meshComponent = m_Registry.get<MeshComponent>(entity);
	if (!m_Registry.all_of<InstanceTagComponent>(entity))
    {
        InstanceTagComponent& instanceTag{m_Registry.emplace<InstanceTagComponent>(entity)};

		auto resource = std::make_shared<RenderSys::Resource>();
		resource->SetBuffer(RenderSys::Resource::BufferIndices::INSTANCE_BUFFER_INDEX, instanceTag.GetInstanceBuffer()->GetBuffer());
		resource->Init();
		for (auto &subMesh : meshComponent.m_Mesh->subMeshes)
		{
			subMesh.m_Resource = resource;
		}
    }

	auto& instanceTagComp = m_Registry.get<InstanceTagComponent>(entity);

	const auto name = meshComponent.m_Name + "_instance" + std::to_string(instanceIndex + 1);
	auto instanceEntity = CreateEntity(name);
	m_sceneGraph.CreateNode(parentNodeIndex, instanceEntity, name);
	RenderSys::TransformComponent& instanceTransform{m_Registry.get<RenderSys::TransformComponent>(instanceEntity)};
	assert(instanceTagComp.GetInstanceBuffer() != nullptr);
	instanceTransform.SetInstance(instanceTagComp.GetInstanceBuffer(), instanceIndex);
	instanceTransform.SetScale(glm::vec3(0.05f));
	instanceTransform.SetTranslation(translation);
	instanceTransform.SetMat4Global();
	instanceTagComp.AddInstance(instanceEntity);
	m_Registry.emplace<RenderSys::MeshComponent>(instanceEntity, "", meshComponent.m_Mesh);
	instanceTagComp.GetInstanceBuffer()->Update();
}

} // namespace Hazel