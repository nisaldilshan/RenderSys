#include "Scene.h"
#include <RenderSys/Resource.h>
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/LightComponents.h>
#include <RenderSys/Components/CameraComponents.h>
#include <RenderSys/Utils/threadpool.hpp>
#include <resources/Shaders/ShaderResource.h>
#include <algorithm>
#include <iostream>

namespace RenderSys
{

namespace
{
// Enough independent subtrees per worker that uneven subtree sizes still even out across the pool.
constexpr size_t SUBTREES_PER_WORKER = 4;
}

Scene::Scene() 
	: m_Registry()
	, m_sceneGraph()
	, m_rootNodeIndex(m_sceneGraph.CreateRootNode(CreateEntity("RootNode"), "RootNode"))
	, m_instancedRootNodeIndex(m_sceneGraph.CreateRootNode(CreateEntity("InstancedRootNode"), "InstancedRootNode"))
	, m_threadPool(std::make_unique<vks::ThreadPool>())
{
	m_threadPool->setThreadCount(std::max(1u, std::thread::hardware_concurrency()));
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
	m_copyInstanceGroups.clear();
}

void Scene::Update()
{
	// Update the transform cache for the root node
	UpdateTransformCacheParallel(m_instancedRootNodeIndex);

	// Change callbacks mutate objects outside the transform (e.g. the camera), so they run here, never on the workers.
	auto transformView = m_Registry.view<TransformComponent>();
	for (auto entity : transformView)
	{
		transformView.get<TransformComponent>(entity).FlushChangeNotification();
	}

	// Upload each instance buffer once per frame, after every transform that writes into it is final.
	// Copies share buffers, but Update() only writes while the buffer is dirty, so each one is uploaded once.
	auto instanceView = m_Registry.view<InstanceTagComponent>();
	for (auto entity : instanceView)
	{
		instanceView.get<InstanceTagComponent>(entity).GetInstanceBuffer()->Update();
	}
}

void Scene::UpdateTransformCacheParallel(uint32_t const rootNodeIndex)
{
	struct PendingSubtree
	{
		uint32_t nodeIndex;
		glm::mat4 parentMat4;
		bool parentDirtyFlag;
	};

	const size_t workerCount = m_threadPool->threads.size();
	const size_t targetSubtrees = workerCount * SUBTREES_PER_WORKER;

	// Walk breadth-first on this thread until the frontier holds enough subtrees to share out. Every frontier
	// node's parent is already final, so the subtrees are independent of each other. Small scenes finish here.
	std::vector<PendingSubtree> frontier{{rootNodeIndex, glm::mat4(1.0f), false}};
	while (!frontier.empty() && frontier.size() < targetSubtrees)
	{
		std::vector<PendingSubtree> nextLevel;
		for (const auto& pending : frontier)
		{
			const auto& node = m_sceneGraph.GetNodeUnsynchronized(pending.nodeIndex);
			bool dirtyFlag = pending.parentDirtyFlag;
			const glm::mat4& mat4Global = UpdateNodeTransform(node.GetGameObject(), pending.parentMat4, dirtyFlag);
			for (uint32_t index = 0; index < node.Children(); index++)
			{
				nextLevel.push_back({node.GetChild(index), mat4Global, dirtyFlag});
			}
		}
		frontier = std::move(nextLevel);
	}

	if (frontier.empty())
	{
		return;
	}

	// Neighbouring siblings tend to be similar in size (e.g. copies of one model), so interleave them across workers.
	for (size_t worker = 0; worker < workerCount; worker++)
	{
		m_threadPool->threads[worker]->addJob([this, &frontier, worker, workerCount]()
		{
			for (size_t index = worker; index < frontier.size(); index += workerCount)
			{
				const auto& pending = frontier[index];
				UpdateTransformCache(pending.nodeIndex, pending.parentMat4, pending.parentDirtyFlag);
			}
		});
	}
	m_threadPool->wait();
}

// Thread-safe for disjoint subtrees: the scene graph and registry are only read, and each node writes only its own
// transform (plus its own slot in a shared instance buffer). Transform change callbacks are deferred to Scene::Update.
void Scene::UpdateTransformCache(uint32_t const nodeIndex, glm::mat4 const &parentMat4, bool parentDirtyFlag)
{
	const auto& node = m_sceneGraph.GetNodeUnsynchronized(nodeIndex);
	bool dirtyFlag = parentDirtyFlag;
	const glm::mat4& mat4Global = UpdateNodeTransform(node.GetGameObject(), parentMat4, dirtyFlag);
	for (uint32_t index = 0; index < node.Children(); index++)
	{
		UpdateTransformCache(node.GetChild(index), mat4Global, dirtyFlag);
	}
}

// dirtyFlag carries the parent's state in and this node's state out, for its children to inherit.
const glm::mat4& Scene::UpdateNodeTransform(entt::entity const gameObject, glm::mat4 const &parentMat4, bool& dirtyFlag)
{
	// registry.get on an existing storage is a lookup only, so concurrent calls are safe.
	auto& transform = m_Registry.get<TransformComponent>(gameObject);
	dirtyFlag = transform.GetDirtyFlag() || dirtyFlag;

	if (dirtyFlag)
	{
		transform.SetMat4Global(parentMat4);
	}

	return transform.GetMat4Global();
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

void Scene::AddInstanceOfSubTree(const uint32_t instanceIndex, const glm::vec3& pos, const uint32_t subTreeNodeIndex, uint32_t parent)
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
			AddInstanceOfSubTree(instanceIndex, pos, childNodeIndex, parent);
		}
	}
}

void Scene::AddMeshInstanceOfEntity(const uint32_t instanceIndex, entt::entity& entity, const glm::vec3& translation, const uint32_t parentNodeIndex)
{
	auto& meshComponent = m_Registry.get<MeshComponent>(entity);
	if (!m_Registry.all_of<InstanceTagComponent>(entity))
    {
        InstanceTagComponent& instanceTag{m_Registry.emplace<InstanceTagComponent>(entity, std::make_shared<InstanceBuffer>())};

		auto resource = std::make_shared<RenderSys::Resource>();
		resource->SetBuffer(RenderSys::Resource::BufferIndices::INSTANCE_BUFFER_INDEX, instanceTag.GetInstanceBuffer()->GetBuffer());
		resource->Init();
		for (auto &subMeshResource : meshComponent.m_SubMeshResources)
		{
			subMeshResource = resource;
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
	instanceTransform.UpdateMat4Global();
	instanceTagComp.AddInstance(instanceEntity);
	m_Registry.emplace<RenderSys::MeshComponent>(instanceEntity, "", meshComponent.m_Mesh);
	instanceTagComp.GetInstanceBuffer()->Update();
}

void Scene::AddCopyOfSubTree(const uint32_t copyIndex, const glm::vec3& pos, const uint32_t subTreeNodeIndex, uint32_t parent)
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
			AddCopyOfEntity(copyIndex, nodeEntity, pos, parent);
		}
		else
		{
			if (subTreeNodeIndex == m_rootNodeIndex)
			{
				// need a proper entity copy mechanism here.
				const auto name = childNode.GetName() + "_copy" + std::to_string(copyIndex + 1);
				auto instanceModelTop = CreateEntity(name);
				parent = m_sceneGraph.CreateNode(m_instancedRootNodeIndex, instanceModelTop, name);
			}
			AddCopyOfSubTree(copyIndex, pos, childNodeIndex, parent);
		}
	}
}

void Scene::AddCopyOfEntity(const uint32_t copyIndex, entt::entity &entity, const glm::vec3 &translation, const uint32_t parentNodeIndex)
{
	auto& meshComponent = m_Registry.get<MeshComponent>(entity);
	const auto name = meshComponent.m_Name + "_copy" + std::to_string(copyIndex + 1);
	auto copy = CreateEntity(name);
	m_sceneGraph.CreateNode(parentNodeIndex, copy, name);

	// Take the next free slot in this source's shared instance buffers, starting a new block when the last is full.
	auto& copyGroup = m_copyInstanceGroups[entity];
	const uint32_t slot = copyGroup.m_copyCount % MAX_INSTANCE;
	if (slot == 0)
	{
		auto instanceBuffer = std::make_shared<InstanceBuffer>();
		auto resource = std::make_shared<RenderSys::Resource>();
		resource->SetBuffer(RenderSys::Resource::BufferIndices::INSTANCE_BUFFER_INDEX, instanceBuffer->GetBuffer());
		resource->Init();
		copyGroup.m_blocks.push_back({instanceBuffer, resource});
	}
	copyGroup.m_copyCount++;
	const auto& block = copyGroup.m_blocks.back();

	// Share the immutable mesh asset; per-entity resource bindings live on the component.
	// Each copy is still drawn on its own, reading only its slot of the shared buffer.
	auto& meshComponentCopy = m_Registry.emplace<MeshComponent>(copy, meshComponent.m_Name, meshComponent.m_Mesh);
	std::fill(meshComponentCopy.m_SubMeshResources.begin(), meshComponentCopy.m_SubMeshResources.end(), block.m_resource);
	meshComponentCopy.m_FirstInstance = slot;

	InstanceTagComponent& instanceTag{m_Registry.emplace<InstanceTagComponent>(copy, block.m_instanceBuffer)};
	instanceTag.AddInstance(copy);

	// The buffer is uploaded by Scene::Update once all transforms writing into it are final.
	RenderSys::TransformComponent& copyTransform{m_Registry.get<RenderSys::TransformComponent>(copy)};
	copyTransform.SetInstance(block.m_instanceBuffer, slot);
	copyTransform.SetScale(glm::vec3(0.05f));
	copyTransform.SetTranslation(translation);
	copyTransform.UpdateMat4Global();
}

void Scene::AddDirectionalLight(const glm::vec3 &direction, const glm::vec3 &position, const glm::vec3 &color)
{
	static uint32_t lightIndex = 0;
	const auto name = "DirectionalLight" + std::to_string(lightIndex++);
	auto lightEntity = CreateEntity(name);
	m_Registry.emplace<DirectionalLightComponent>(lightEntity);
	m_sceneGraph.CreateNode(m_instancedRootNodeIndex, lightEntity, name);

	auto& dirLightComp = m_Registry.get<DirectionalLightComponent>(lightEntity);
	dirLightComp.m_Color = color;

	auto& transformComp = m_Registry.get<TransformComponent>(lightEntity);
	transformComp.SetRotation(direction);
	transformComp.SetTranslation(position);
}

entt::entity Scene::AddCamera(std::shared_ptr<RenderSys::ICamera> camera)
{
	static uint32_t cameraIndex = 0;
	const auto name = "Camera" + std::to_string(cameraIndex++);
	auto cameraEntity = CreateEntity(name);
	m_Registry.emplace<PerspectiveCameraComponent>(cameraEntity, m_Registry.get<TransformComponent>(cameraEntity));
	m_sceneGraph.CreateNode(m_instancedRootNodeIndex, cameraEntity, name);

	auto& cameraComp = m_Registry.get<PerspectiveCameraComponent>(cameraEntity);
	auto perspect = std::dynamic_pointer_cast<PerspectiveCamera>(camera);
	if (!perspect)
	{
		std::cerr << "Error: Camera is not a PerspectiveCamera!" << std::endl;
		assert(false);
		return cameraEntity;
	}
	cameraComp.m_Camera = perspect;
	return cameraEntity;
}

} // namespace Hazel