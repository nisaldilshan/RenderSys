#pragma once

#include <RenderSys/Scene/Scene.h>

namespace RenderSys
{

class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const std::shared_ptr<Scene> scene);
    ~SceneHierarchyPanel() = default;

    void SetContext(const std::shared_ptr<Scene> scene);
    void OnImGuiRender();

    entt::entity GetSelectedEntity() const { return m_SelectionContext; }
    void SetSelectedEntity(entt::entity entity);
private:
    void DrawEntityNodes();
    void DrawEntityNodeRecursive(SceneGraph::TreeNode& node);
    void DrawComponents(entt::entity entity);
    
    std::shared_ptr<Scene> m_Context;
    entt::entity m_SelectionContext;

    friend class Scene;
};
    
} // namespace 
