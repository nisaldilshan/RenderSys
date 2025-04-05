#include "Material.h"

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanMaterial.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPUMaterial.h"
#else
static_assert(false);
#endif

namespace RenderSys
{

MaterialDescriptor::MaterialDescriptor()
{
}

Material::Material()
    : m_materialProperties()
    , m_MaterialDescriptor(std::make_shared<MaterialDescriptor>())
{

}


Material::~Material()
{
}

void Material::Init()
{
}

} // namespace RenderSys