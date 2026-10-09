#include "MeshComponent.h"

namespace RenderSys
{

MeshComponent::MeshComponent(std::string const &name, std::shared_ptr<Mesh> mesh)
    : m_Name(name), m_Mesh(mesh)
{
    assert(m_Mesh != nullptr);
    m_SubMeshResources.resize(m_Mesh->subMeshes.size());
}

}