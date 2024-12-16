#include "SceneNode.h"

#include <iostream>
#include <string>



SceneNode::SceneNode(int nodeNum)
    : m_nodeNum(nodeNum)
{
}

int SceneNode::getNodeNum()
{
    return m_nodeNum;
}

void SceneNode::setNodeName(std::string name)
{
    m_nodeName = name;
}

void SceneNode::setScale(glm::vec3 scale)
{
}

void SceneNode::setTranslation(glm::vec3 translation)
{
}

void SceneNode::setRotation(glm::quat rotation)
{
}

void SceneNode::printHierarchy(int indent)
{
    std::string indendString = "";
    for (int i = 0; i < indent; ++i) {
        indendString += " ";
    }
    indendString += "-";
    std::string printStr = indendString + " child : " + std::to_string(m_nodeNum) + " (" + m_nodeName + ")";
    std::cout << printStr << std::endl;

    for (const auto& childNode : m_childNodes) {
        childNode->printHierarchy(indent + 1);
    }
}
