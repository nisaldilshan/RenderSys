#pragma once

#include <array>
#include <memory>
#include <glm/ext.hpp>

namespace RenderSys
{
#if (RENDERER_BACKEND == 1)
class OpenGLMaterialDescriptor;
typedef OpenGLMaterialDescriptor MaterialDescriptorType;
#elif (RENDERER_BACKEND == 2)
class VulkanMaterialDescriptor;
typedef VulkanMaterialDescriptor MaterialDescriptorType;
#elif (RENDERER_BACKEND == 3)
class WebGPUMaterialDescriptor;
typedef WebGPUMaterialDescriptor MaterialDescriptorType;
#else
static_assert(false);
#endif

#include <Resources/Shaders/ShaderMaterial.h>

enum MaterialFeatures // bitset
{
    HAS_DIFFUSE_MAP = GLSL_HAS_DIFFUSE_MAP,
    HAS_NORMAL_MAP = GLSL_HAS_NORMAL_MAP,
    HAS_ROUGHNESS_MAP = GLSL_HAS_ROUGHNESS_MAP,
    HAS_METALLIC_MAP = GLSL_HAS_METALLIC_MAP,
    HAS_ROUGHNESS_METALLIC_MAP = GLSL_HAS_ROUGHNESS_METALLIC_MAP,
    HAS_EMISSIVE_COLOR = GLSL_HAS_EMISSIVE_COLOR,
    HAS_EMISSIVE_MAP = GLSL_HAS_EMISSIVE_MAP
};

enum TextureIndices
{
    DIFFUSE_MAP_INDEX = 0,
    NORMAL_MAP_INDEX,
    ROUGHNESS_MAP_INDEX,
    METALLIC_MAP_INDEX,
    ROUGHNESS_METALLIC_MAP_INDEX,
    EMISSIVE_MAP_INDEX,
    DUMMY_MAP_INDEX, // used for default texture, when diffuse map is not provided.
    NUM_TEXTURES
};

class Texture; // forward declaration

// fixed-size array for material textures
typedef std::array<std::shared_ptr<Texture>, NUM_TEXTURES> MaterialTextures;

enum class ShaderWorkflow : int32_t
{ 
    PBR_WORKFLOW_METALLIC_ROUGHNESS = 0, 
    PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1 
};
static_assert(sizeof(ShaderWorkflow) == 4);

class MaterialDescriptor
{
public:
    MaterialDescriptor();
    ~MaterialDescriptor() = default;

    MaterialDescriptor(const MaterialDescriptor&) = delete;
    MaterialDescriptor& operator=(const MaterialDescriptor&) = delete;
    MaterialDescriptor(MaterialDescriptor&&) = delete;
    MaterialDescriptor& operator=(MaterialDescriptor&&) = delete;

    void Init(MaterialTextures& textures);
    void Destroy();

    MaterialDescriptorType* GetPlatformDescriptor() const { return m_platformDescriptor.get(); }
private:
    std::unique_ptr<MaterialDescriptorType> m_platformDescriptor{nullptr};
    ShaderWorkflow m_shaderWorkflow{ShaderWorkflow::PBR_WORKFLOW_METALLIC_ROUGHNESS};
};


class Material
{
public:
    Material();
    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = delete;
    Material& operator=(Material&&) = delete;

    void Init();
    
    void SetMaterialProperties(const MaterialProperties& matProps) { m_materialProperties = matProps; }
    void SetMaterialTexture(const TextureIndices textureIndex, std::shared_ptr<Texture> texture);

    std::shared_ptr<MaterialDescriptor> GetDescriptor() const { return m_MaterialDescriptor; }
    const MaterialProperties& GetMaterialProperties() const { return m_materialProperties; }
private:
    MaterialProperties m_materialProperties;
    MaterialTextures m_materialTextures;
    std::shared_ptr<MaterialDescriptor> m_MaterialDescriptor;
};

} // namespace RenderSys