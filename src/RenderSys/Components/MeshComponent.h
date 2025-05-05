#pragma once

#include <string>
#include <memory>
#include <RenderSys/Scene/SceneNode.h>

namespace RenderSys
{

class MeshComponent
{
public:
    MeshComponent() = delete;
    MeshComponent(std::string const& name, std::shared_ptr<Mesh> mesh, bool enabled = true);
    ~MeshComponent() = default;
    MeshComponent(const MeshComponent&) = delete;
    MeshComponent &operator=(const MeshComponent&) = delete;
    MeshComponent(MeshComponent&&) = delete;
    MeshComponent &operator=(MeshComponent&&) = delete;

    std::string m_Name;
    std::shared_ptr<Mesh> m_Mesh;
    bool m_Enabled{false};
};
   
} // namespace RenderSys
