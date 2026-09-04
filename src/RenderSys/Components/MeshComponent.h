#pragma once

#include <string>
#include <memory>
#include <vector>
#include <RenderSys/Scene/Mesh.h>

namespace RenderSys
{

class Resource;

class MeshComponent
{
public:
    MeshComponent() = delete;
    MeshComponent(std::string const& name, std::shared_ptr<Mesh> mesh);
    ~MeshComponent() = default;
    MeshComponent(const MeshComponent&) = delete;
    MeshComponent &operator=(const MeshComponent&) = delete;
    MeshComponent(MeshComponent&&) = delete;
    MeshComponent &operator=(MeshComponent&&) = delete;

    std::string m_Name;
    std::shared_ptr<Mesh> m_Mesh;

    // Entity-specific resource bindings, one per entry in m_Mesh->subMeshes.
    // Kept off the shared Mesh asset so copies/instances never share this state.
    std::vector<std::shared_ptr<Resource>> m_SubMeshResources;
};

} // namespace RenderSys
