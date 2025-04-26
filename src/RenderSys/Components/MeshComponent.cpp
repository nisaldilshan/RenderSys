#include "MeshComponent.h"

namespace RenderSys
{

MeshComponent::MeshComponent(std::string const &name, std::shared_ptr<Mesh> mesh, bool enabled)
    : m_Name(name), m_Mesh(mesh), m_Enabled(enabled)
{
}

}