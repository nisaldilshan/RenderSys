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

// material
#define GLSL_HAS_DIFFUSE_MAP (0x1 << 0x0)
#define GLSL_HAS_NORMAL_MAP (0x1 << 0x1)
#define GLSL_HAS_ROUGHNESS_MAP (0x1 << 0x2)
#define GLSL_HAS_METALLIC_MAP (0x1 << 0x3)
#define GLSL_HAS_ROUGHNESS_METALLIC_MAP (0x1 << 0x4)
#define GLSL_HAS_EMISSIVE_COLOR (0x1 << 0x5)
#define GLSL_HAS_EMISSIVE_MAP (0x1 << 0x6)

#define GLSL_NUM_MULTI_MATERIAL 4
    
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

struct MaterialProperties
{ // align data to blocks of 16 bytes
    // byte 0 to 15
    uint32_t m_features{0};
    float m_roughness{0.0f};
    float m_metallic{0.0f};
    float m_NormalMapIntensity{1.0f};

    // byte 16 to 31
    glm::vec4 m_baseColor{1.0f, 1.0f, 1.0f, 1.0f};

    // byte 32 to 47
    glm::vec3 m_EmissiveColor{0.0f, 0.0f, 0.0f};
    float m_EmissiveStrength{1.0f};
};

enum TextureIndices
{
    DIFFUSE_MAP_INDEX = 0,
    NORMAL_MAP_INDEX,
    ROUGHNESS_METALLIC_MAP_INDEX,
    NUM_TEXTURES
};

class Texture; // forward declaration

// fixed-size array for material textures
typedef std::array<std::shared_ptr<Texture>, NUM_TEXTURES> MaterialTextures;

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

    MaterialDescriptorType* GetPlatformDescriptor() const { return m_platformDescriptor.get(); }
private:
    std::unique_ptr<MaterialDescriptorType> m_platformDescriptor{nullptr};
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

    std::shared_ptr<MaterialDescriptor> GetMaterialDescriptor() const { return m_MaterialDescriptor; }
    const MaterialProperties& GetMaterialProperties() const { return m_materialProperties; }
private:
    MaterialProperties m_materialProperties;
    MaterialTextures m_materialTextures;
    std::shared_ptr<MaterialDescriptor> m_MaterialDescriptor;
};

} // namespace RenderSys