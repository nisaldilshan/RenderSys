#version 460

#include "ShaderMaterial.h"
#include "metallic-roughness.glsl"
#include "pbr-material.glsl"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    float time;
} ubo;

layout(set = 0, binding = 1) uniform LightingUniforms {
    vec4 directions[2];
    vec4 colors[2];
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

    vec3 color = pbrAmbient(surface);
    for (int i = 0; i < lightingUbo.directions.length(); ++i) 
    {
        color += pbrDirectLight(surface, V, normalize(lightingUbo.directions[i].xyz), lightingUbo.colors[i].rgb);
    }

    out_color = vec4(color, surface.alpha);
    out_color = pow(out_color, vec4(1.0/2.2));
}
