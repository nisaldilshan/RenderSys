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

void MaterialDescriptor::Init(MaterialTextures& textures)
{
    m_platformDescriptor = std::make_unique<MaterialDescriptorType>(textures);
}

void MaterialDescriptor::Destroy()
{
    if (m_platformDescriptor)
    {
        m_platformDescriptor.reset();
    }
}

Material::Material()
    : m_materialProperties()
    , m_MaterialDescriptor(std::make_shared<MaterialDescriptor>())
{

}


Material::~Material()
{
    if (m_MaterialDescriptor)
    {
        m_MaterialDescriptor->Destroy();
        m_MaterialDescriptor.reset();
    }
}

void Material::Init()
{
    m_MaterialDescriptor->Init(m_materialTextures);
}

void Material::SetMaterialTexture(const TextureIndices textureIndex, std::shared_ptr<Texture> texture)
{
    m_materialTextures[textureIndex] = texture;
}

} // namespace RenderSys