#include "SceneNode.h"

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

