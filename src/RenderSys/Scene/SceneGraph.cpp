#include "SceneGraph.h"
#include <iostream>

namespace RenderSys
{
SceneGraph::TreeNode::TreeNode(entt::entity gameObject, const std::string& name) : m_GameObject(gameObject), m_Name(name)
{
}

SceneGraph::TreeNode::TreeNode(TreeNode const& other)
    : m_GameObject(other.m_GameObject), m_Name(other.m_Name), m_Children(other.m_Children)
{
}

SceneGraph::TreeNode::~TreeNode() {}

entt::entity SceneGraph::TreeNode::GetGameObject() const { return m_GameObject; }

const std::string& SceneGraph::TreeNode::GetName() const { return m_Name; }

uint32_t SceneGraph::TreeNode::Children() const { return m_Children.size(); }

uint32_t SceneGraph::TreeNode::GetChild(uint32_t const childIndex) const { return m_Children[childIndex]; }

uint32_t SceneGraph::TreeNode::AddChild(uint32_t const nodeIndex)
{
    uint32_t childIndex = m_Children.size();
    m_Children.push_back(nodeIndex);
    return childIndex;
}

uint32_t SceneGraph::CreateNode(uint32_t parentNode, entt::entity const gameObject, std::string const& name)
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    uint32_t nodeIndex = m_Nodes.size();
    m_Nodes.push_back({gameObject, name});
    //dictionary.Insert(name, gameObject);
    m_MapFromGameObjectToNode[gameObject] = nodeIndex;
    m_Nodes[parentNode].AddChild(nodeIndex);
    return nodeIndex;
}

uint32_t SceneGraph::CreateRootNode(entt::entity const gameObject, std::string const& name)
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    uint32_t nodeIndex = m_Nodes.size();
    m_Nodes.push_back({gameObject, name});
    //dictionary.Insert(name, gameObject);
    m_MapFromGameObjectToNode[gameObject] = nodeIndex;
    return nodeIndex;
}

void SceneGraph::TraverseLog(uint32_t nodeIndex, uint32_t indent) const
{
    std::string indentStr(indent, ' ');
    const TreeNode& treeNode = m_Nodes[nodeIndex];
    std::cout << indentStr << "game object `" << static_cast<uint32_t>(treeNode.GetGameObject()) << "`, name: `" << treeNode.GetName() << "`" << std::endl;
    for (uint32_t childIndex = 0; childIndex < treeNode.Children(); ++childIndex)
    {
        TraverseLog(treeNode.GetChild(childIndex), indent + 4);
    }
}

SceneGraph::TreeNode& SceneGraph::GetNode(uint32_t const nodeIndex)
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    return m_Nodes[nodeIndex];
}

SceneGraph::TreeNode& SceneGraph::GetNodeByGameObject(entt::entity const gameObject)
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    uint32_t nodeIndex = m_MapFromGameObjectToNode[gameObject];
    return m_Nodes[nodeIndex];
}

SceneGraph::TreeNode& SceneGraph::GetRoot()
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    //CORE_ASSERT(m_Nodes.size(), "SceneGraph::GetRoot(): scene graph is empty");
    return m_Nodes[SceneGraph::ROOT_NODE];
}

uint32_t SceneGraph::GetTreeNodeIndex(entt::entity const gameObject)
{
    std::lock_guard<std::mutex> guard(m_MutexSceneGraph);
    uint32_t returnValue = NODE_INVALID;

    if (m_MapFromGameObjectToNode.find(gameObject) != m_MapFromGameObjectToNode.end())
    {
        returnValue = m_MapFromGameObjectToNode[gameObject];
    }
    return returnValue;
}

} // namespace RenderSys
