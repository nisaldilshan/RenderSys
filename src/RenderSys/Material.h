#pragma once

#include <array>
#include <memory>

namespace RenderSys
{

struct PbrMaterialProperties
{ // align data to blocks of 16 bytes
    // byte 0 to 15
    uint m_Features{0};
    float m_Roughness{0.0f};
    float m_Metallic{0.0f};
    float m_NormalMapIntensity{1.0f};

    // byte 16 to 31
    glm::vec4 m_DiffuseColor{1.0f, 1.0f, 1.0f, 1.0f};

    // byte 32 to 47
    glm::vec3 m_EmissiveColor{0.0f, 0.0f, 0.0f};
    float m_EmissiveStrength{1.0f};
};

class Material
{
public:
    Material();
    ~Material();
private:
    PbrMaterialProperties m_PbrMaterialProperties;
    std::array<std::shared_ptr<Texture>, NUM_TEXTURES> m_MaterialTextures
};

} // namespace RenderSys