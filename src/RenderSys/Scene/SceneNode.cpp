#include "SceneNode.h"

#include <iostream>
#include <string>
#include <RenderSys/Components/EntityRegistry.h>
#include <RenderSys/Components/TransformComponent.h>

namespace RenderSys
{
namespace Model
{

ModelNode::ModelNode(int nodeNum)
    : m_nodeNum(nodeNum)
{
}

int ModelNode::getNodeNum()
{
    return m_nodeNum;
}

void ModelNode::setNodeName(std::string name)
{
    m_nodeName = name;
}

void ModelNode::setMesh(RenderSys::Mesh mesh)
{
    m_mesh = mesh;
}

entt::entity &ModelNode::getEntity()
{
    assert(m_entity != entt::null);
    return m_entity;
}

void ModelNode::calculateNodeMatrix(glm::mat4 parentNodeMatrix)
{
    assert(m_entity != entt::null);
    auto& registry = EntityRegistry::Get();
    auto& transform = registry.get<RenderSys::TransformComponent>(m_entity);
    transform.SetMat4Global(parentNodeMatrix);
}

void ModelNode::calculateJointMatrices(const std::vector<glm::mat4> &inverseBindMatrices, const std::vector<int> &nodeToJoint, std::vector<glm::mat4> &jointMatrices)
{
    auto placeHolder = nodeToJoint.at(m_nodeNum);
    jointMatrices.at(placeHolder) = getNodeMatrix() * inverseBindMatrices.at(placeHolder);

    for (const auto& childNode : m_childNodes)
    {
        childNode->calculateJointMatrices(inverseBindMatrices, nodeToJoint, jointMatrices);
    }
}

glm::mat4 ModelNode::getNodeMatrix()
{
    assert(m_entity != entt::null);
    auto& registry = EntityRegistry::Get();
    auto& transform = registry.get<RenderSys::TransformComponent>(m_entity);
    return transform.GetMat4Global();
}

Mesh ModelNode::getMesh()
{
    return m_mesh;
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

} // namespace Model

} // namespace RenderSys
