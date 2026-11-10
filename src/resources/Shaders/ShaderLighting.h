#ifndef SHADERLIGHTING_H
#define SHADERLIGHTING_H

// Lighting uniform block shared by C++ and GLSL (set 0, binding 1 of the PBR examples).
// Every member is 16 bytes or a mat4, so the C++ layout equals std140 without padding.

#ifdef __cplusplus
    #include <glm/glm.hpp>
    typedef glm::vec4 vec4;
    typedef glm::ivec4 ivec4;
    typedef glm::mat4 mat4;
#endif

#define GLSL_MAX_DIRECTIONAL_LIGHTS 2

// Tone curves, selected by LightingProperties::m_settings.y
#define GLSL_TONEMAP_NONE 0         // clamp to [0, 1]
#define GLSL_TONEMAP_ACES_FITTED 1  // Stephen Hill's fit of the ACES RRT and sRGB ODT
#define GLSL_TONEMAP_PBR_NEUTRAL 2  // Khronos PBR Neutral

// Flags in LightingProperties::m_settings.z
// Set when a later pass applies exposure, the tone curve and the sRGB encoding;
// the forward pass then writes linear scene values, so nothing is applied twice.
#define GLSL_DISPLAY_TRANSFORM_IN_POST (0x1 << 0x0)

// What the shadow-mapping examples draw in place of the lit scene, selected by LightingProperties::m_settings.w
#define GLSL_SHADOW_DEBUG_NONE 0       // the lit scene
#define GLSL_SHADOW_DEBUG_SUN_SHADOW 1 // the sun's visibility: white where lit, black where shadowed
#define GLSL_SHADOW_DEBUG_TEXELS 2     // the shadow map's texel grid over the scene, red outside the map

struct LightingProperties
{
    // bytes 0 to 31: xyz is the unit vector towards each light, w is unused
    vec4 m_lightDirections[GLSL_MAX_DIRECTIONAL_LIGHTS];

    // bytes 32 to 63: rgb is the illuminance on a surface facing each light, w is unused.
    // A white Lambert surface facing the light reflects rgb / pi.
    vec4 m_lightColors[GLSL_MAX_DIRECTIONAL_LIGHTS];

    // bytes 64 to 79: rgb is the radiance of the sky, the hemisphere above the horizon (+y), w is unused
    vec4 m_skyColor;

    // bytes 80 to 95: rgb is the radiance of the ground, the hemisphere below the horizon, w is unused
    vec4 m_groundColor;

    // bytes 96 to 159: world to light clip space for light 0, read only by shadow-mapping shaders
    mat4 m_lightViewProjection;

    // bytes 160 to 175: x is the exposure as a linear scale (2^EV), yzw are unused
    vec4 m_exposure;

    // bytes 176 to 191: x is the number of lights in use, y is a GLSL_TONEMAP_* curve,
    // z holds GLSL_DISPLAY_* flags, w is a GLSL_SHADOW_DEBUG_* view
    ivec4 m_settings;

    // bytes 192 to 207, read only by shadow-mapping shaders: x is how far a receiver moves along its normal
    // before the shadow lookup, in world units (the normal offset), y is the PCF kernel radius in texels
    // (0 is one hardware 2x2 tap, 1 is 3x3 taps), z and w are unused
    vec4 m_shadowParams;
};

#endif // SHADERLIGHTING_H
