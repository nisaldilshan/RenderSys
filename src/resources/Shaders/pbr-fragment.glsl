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

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec4 in_tangent;

layout (location = 0) out vec4 out_color;

void main()
{
    PbrSurface surface = getPbrSurface(in_uv, in_normal, in_tangent);
    vec3 V = normalize(in_viewDirection);

    vec3 color = pbrAmbient(surface, V, lightingUbo.m_lighting.m_skyColor.rgb, lightingUbo.m_lighting.m_groundColor.rgb);
    int lightCount = min(lightingUbo.m_lighting.m_settings.x, GLSL_MAX_DIRECTIONAL_LIGHTS);
    for (int i = 0; i < lightCount; ++i)
    {
        color += pbrDirectLight(surface, V, normalize(lightingUbo.m_lighting.m_lightDirections[i].xyz), lightingUbo.m_lighting.m_lightColors[i].rgb);
    }

    out_color = vec4(applyDisplayTransform(color, lightingUbo.m_lighting), surface.alpha);
}
