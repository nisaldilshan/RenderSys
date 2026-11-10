#version 460

#include "ShaderMaterial.h"
#include "ShaderLighting.h"
#include "metallic-roughness.glsl"
#include "pbr-material.glsl"
#include "tonemap.glsl"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    float time;
} ubo;

layout(std140, set = 0, binding = 1) uniform LightingUniforms {
    LightingProperties m_lighting;
} lightingUbo;

// The sampler compares: texture(shadowMap, vec3(uv, depth)) is 1 where depth is at or in front of the
// stored depth and 0 behind it, averaged over the 2x2 nearest texels. Outside the map the border reads
// as the far plane, so receivers there are lit.
layout(set = 0, binding = 2) uniform sampler2DShadow shadowMap;

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec4 in_tangent;
layout (location = 4) in vec3 in_shadowCoord;

layout (location = 0) out vec4 out_color;

bool isInShadowMap(vec3 shadowCoord)
{
    return all(greaterThanEqual(shadowCoord, vec3(0.0))) && all(lessThanEqual(shadowCoord, vec3(1.0)));
}

// The sun's visibility, 1 where lit and 0 where shadowed: percentage-closer filtering over
// (2r + 1)^2 taps one texel apart, each a hardware 2x2 compare, for a soft (2r + 2)-texel edge.
float sunVisibility(vec3 shadowCoord)
{
    // Past the far plane nothing was drawn into the map to compare against
    if (shadowCoord.z > 1.0)
        return 1.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    int radius = int(lightingUbo.m_lighting.m_shadowParams.y + 0.5);
    float visibility = 0.0;
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            visibility += texture(shadowMap, vec3(shadowCoord.xy + vec2(x, y) * texelSize, shadowCoord.z));
        }
    }
    float tapCount = float((2 * radius + 1) * (2 * radius + 1));
    return visibility / tapCount;
}

void main()
{
    PbrSurface surface = getPbrSurface(in_uv, in_normal, in_tangent);
    vec3 V = normalize(in_viewDirection);

    // The shadow map is rendered from light 0, the sun. It blocks only the sun's light:
    // the sky and ground light from pbrAmbient still reach shadowed surfaces.
    float sunShadow = sunVisibility(in_shadowCoord);

    vec3 color = pbrAmbient(surface, V, lightingUbo.m_lighting.m_skyColor.rgb, lightingUbo.m_lighting.m_groundColor.rgb);
    int lightCount = min(lightingUbo.m_lighting.m_settings.x, GLSL_MAX_DIRECTIONAL_LIGHTS);
    for (int i = 0; i < lightCount; ++i)
    {
        float shadow = (i == 0) ? sunShadow : 1.0;
        color += shadow * pbrDirectLight(surface, V, normalize(lightingUbo.m_lighting.m_lightDirections[i].xyz), lightingUbo.m_lighting.m_lightColors[i].rgb);
    }
    vec3 displayColor = applyDisplayTransform(color, lightingUbo.m_lighting);

    int debugView = lightingUbo.m_lighting.m_settings.w;
    if (debugView != GLSL_SHADOW_DEBUG_NONE)
    {
        if (debugView == GLSL_SHADOW_DEBUG_SUN_SHADOW)
        {
            displayColor = vec3(sunShadow);
        }
        else if (debugView == GLSL_SHADOW_DEBUG_TEXELS)
        {
            // A checkerboard of the map's texels: their size on screen is the shadow's resolution there,
            // and texel snapping keeps it still on the surfaces while the camera moves
            ivec2 texel = ivec2(floor(in_shadowCoord.xy * vec2(textureSize(shadowMap, 0))));
            displayColor *= ((texel.x + texel.y) & 1) == 0 ? 1.0 : 0.55;
        }
        // Receivers the map does not cover get no shadow
        if (!isInShadowMap(in_shadowCoord))
            displayColor = mix(displayColor, vec3(1.0, 0.0, 0.0), 0.5);
    }

    out_color = vec4(displayColor, surface.alpha);
}