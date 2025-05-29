#pragma once

#include <vector>
#include <mutex>
#include <entt/entt.hpp>

namespace RenderSys
{

class SceneGraph
{
public:
    class TreeNode
    {

    public:
        TreeNode(entt::entity gameObject, const std::string& name);
        TreeNode(TreeNode const& other);
        ~TreeNode();

        const std::string& GetName() const;
        uint32_t GetChild(uint32_t const childIndex) const;
        std::vector<uint32_t>& GetChildren() { return m_Children; }

        uint32_t Children() const;
        entt::entity GetGameObject() const;

    private:
        uint32_t AddChild(uint32_t const nodeIndex);

    private:
        entt::entity m_GameObject;
        std::string m_Name;
        std::vector<uint32_t> m_Children;

        friend SceneGraph;
    };

    static constexpr uint32_t ROOT_NODE = 0;
    static constexpr uint32_t NODE_INVALID = -1;

public:
    SceneGraph() = default;
    ~SceneGraph() = default;
    SceneGraph(SceneGraph const& other) = delete;
    SceneGraph& operator=(SceneGraph const& other) = delete;
    SceneGraph(SceneGraph&& other) = delete;
    SceneGraph& operator=(SceneGraph&& other) = delete;

    uint32_t CreateNode(uint32_t parentNode, entt::entity const gameObject, std::string const& name);
    uint32_t CreateRootNode(entt::entity const gameObject, std::string const& name);
    TreeNode& GetNode(uint32_t const nodeIndex);
    TreeNode& GetNodeByGameObject(entt::entity const gameObject);
    TreeNode& GetRoot();

    uint32_t GetTreeNodeIndex(entt::entity const gameObject);
    void TraverseLog(uint32_t nodeIndex, uint32_t indent = 0) const;

private:
    std::mutex m_MutexSceneGraph;
    std::vector<TreeNode> m_Nodes;
    std::map<entt::entity, uint32_t> m_MapFromGameObjectToNode;
};

} // namespace RenderSys
