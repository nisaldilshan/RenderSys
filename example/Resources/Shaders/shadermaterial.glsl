#ifndef SHADERMATERIAL_H
#define SHADERMATERIAL_H

#ifdef __cplusplus // This macro is defined by C++ compilers
    #include <glm/glm.hpp> // Or your specific glm include path
    typedef glm::vec3 vec3;
    typedef glm::vec4 vec4;
    // Add any other C++/GLSL shared types here, e.g., mat4
    typedef glm::mat4 mat4;
#else // This will be used by GLSL compilers
    // GLSL already has vec3, vec4, mat4 defined
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

struct MaterialProperties
{ 
    int m_features;
    float m_roughness;
    float m_metallic;
    float m_NormalMapIntensity;

    // byte 16 to 31
    vec4 m_baseColor;

    // byte 32 to 47
    vec3 m_EmissiveColor;
    float m_EmissiveStrength;
};

#endif // SHADERMATERIAL_H