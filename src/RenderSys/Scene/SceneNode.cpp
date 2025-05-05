#include "SceneNode.h"

#include <iostream>
#include <string>
#include <RenderSys/Components/TransformComponent.h>

namespace RenderSys
{

ModelNode::ModelNode(int nodeNum, entt::entity entity)
    : m_nodeNum(nodeNum)
    , m_data(std::make_shared<ModelData>())
    , m_entity(entity)
{
    //std::cout << "ModelNode(" << this << ") created with nodeNum: " << m_nodeNum << ", Entity:" << int(m_entity) << std::endl;
}

int ModelNode::getNodeNum()
{
    return m_nodeNum;
}

void ModelNode::setNodeName(std::string name)
{
    m_nodeName = name;
}

entt::entity &ModelNode::getEntity()
{
    assert(m_entity != entt::null);
    return m_entity;
}

void ModelNode::calculateJointMatrices(const std::vector<glm::mat4> &inverseBindMatrices, const std::vector<int> &nodeToJoint
    , std::vector<glm::mat4> &jointMatrices, entt::registry& registry)
{
    auto placeHolder = nodeToJoint.at(m_nodeNum);
    auto& transform = registry.get<RenderSys::TransformComponent>(m_entity);
    jointMatrices.at(placeHolder) = transform.GetMat4Global() * inverseBindMatrices.at(placeHolder);

    for (const auto& childNode : m_childNodes)
    {
        childNode->calculateJointMatrices(inverseBindMatrices, nodeToJoint, jointMatrices, registry);
    }
}

void ModelNode::printHierarchy(int indent)
{
    std::string indendString = "";
    for (int i = 0; i < indent; ++i) {
        indendString += " ";
    }
    indendString += "-";

    std::string printStr;
    if (indent == 0)
    {
        printStr = " root : " + std::to_string(m_nodeNum) + " (" + m_nodeName + ")";
    }
    else
    {
        printStr = indendString + " child : " + std::to_string(m_nodeNum) + " (" + m_nodeName + ")";
    }
    
    std::cout << printStr << std::endl;

    for (const auto& childNode : m_childNodes) {
        childNode->printHierarchy(indent + 1);
    }
}

} // namespace RenderSys
