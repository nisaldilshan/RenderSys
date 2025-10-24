#version 460

#include "ShaderResource.h"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec3 cameraWorldPosition;
    float time;
} ubo;

layout(std140, set = 0, binding = 1) uniform LightingUniforms {
    vec4 directions[1];
    vec4 colors[1];
    mat4 padding[1];
    mat4 viewProjection[1];
} lightingUbo;

struct InstanceData
{
    mat4 m_ModelMatrix;
};

layout(set = 2, binding = 0) readonly buffer InstanceBuffer
{
    InstanceData m_InstanceData[MAX_INSTANCE];
} uboInstanced;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_color;
layout (location = 4) in vec3 in_tangent;

layout (location = 0) out vec3 out_viewDirection;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec3 out_tangent;
layout (location = 4) out vec4 outShadowCoord;

void main() 
{
    mat4 modelMatrix = uboInstanced.m_InstanceData[gl_InstanceIndex].m_ModelMatrix;

    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
    out_viewDirection = ubo.cameraWorldPosition - worldPosition.xyz;
    out_uv = in_uv;
	out_normal = (modelMatrix * vec4(in_normal, 0.0)).xyz;
    out_tangent = (modelMatrix * vec4(in_tangent, 0.0)).xyz;
}