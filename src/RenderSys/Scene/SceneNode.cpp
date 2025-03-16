#include "SceneNode.h"

#include <iostream>
#include <string>

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

void ModelNode::setScale(glm::vec3 scale)
{
    m_scale = scale;
}

void ModelNode::setTranslation(glm::vec3 translation)
{
    m_translation = translation;
}

void ModelNode::setRotation(glm::quat rotation)
{
    m_rotation = rotation;
}

void ModelNode::setMesh(RenderSys::Mesh mesh)
{
    m_mesh = mesh;
}

void ModelNode::calculateLocalTRSMatrix()
{
    glm::mat4 sMatrix = glm::scale(glm::mat4(1.0f), m_scale);
    glm::mat4 rMatrix = glm::mat4_cast(m_rotation);
    glm::mat4 tMatrix = glm::translate(glm::mat4(1.0f), m_translation);
    mLocalTRSMatrix = tMatrix * rMatrix * sMatrix;
}

void ModelNode::calculateNodeMatrix(glm::mat4 parentNodeMatrix)
{
    m_nodeMatrix = parentNodeMatrix * mLocalTRSMatrix;
}

void ModelNode::calculateJointMatrices(const std::vector<glm::mat4> &inverseBindMatrices, const std::vector<int> &nodeToJoint, std::vector<glm::mat4> &jointMatrices)
{
    auto placeHolder = nodeToJoint.at(m_nodeNum);
    jointMatrices.at(placeHolder) = m_nodeMatrix * inverseBindMatrices.at(placeHolder);

    for (const auto& childNode : m_childNodes)
    {
        childNode->calculateJointMatrices(inverseBindMatrices, nodeToJoint, jointMatrices);
    }
}

glm::mat4 ModelNode::getNodeMatrix()
{
    return m_nodeMatrix;
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
