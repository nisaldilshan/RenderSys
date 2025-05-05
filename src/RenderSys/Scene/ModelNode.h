#pragma once
#include <vector>
#include <memory>
#include <string>
#include <RenderSys/Scene/Mesh.h>

namespace RenderSys
{



class ModelNode 
{
  public:
    ModelNode() = delete; 
    ModelNode(int nodeNum, entt::entity entity);
    ~ModelNode() = default;

	ModelNode(const ModelNode&) = delete;
	ModelNode &operator=(const ModelNode&) = delete;
	ModelNode(ModelNode&&) = delete;
	ModelNode &operator=(ModelNode&&) = delete;

    int getNodeNum();
    void setNodeName(std::string name);
    entt::entity& getEntity();

    void calculateJointMatrices(const std::vector<glm::mat4>& inverseBindMatrices, const std::vector<int>& nodeToJoint
        , std::vector<glm::mat4>& jointMatrices, entt::registry& registry);
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<ModelNode>> m_childNodes{};
    std::shared_ptr<MeshData> m_data;

  private:
    int m_nodeNum = 0;
    std::string m_nodeName;
    int m_materialIndex = 0;
    entt::entity m_entity;
};

} // namespace RenderSys