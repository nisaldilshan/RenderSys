#version 460

#include "ShaderResource.h"
#include "ShaderLighting.h"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec3 cameraWorldPosition;
    float time;
} ubo;

layout(std140, set = 0, binding = 1) uniform LightingUniforms {
    LightingProperties m_lighting;
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
layout (location = 4) in vec4 in_tangent;

layout (location = 0) out vec3 out_viewDirection;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec4 out_tangent;
// xy is the shadow map's uv and z the receiver's depth in the light's [0, 1] range. The light's projection
// is orthographic, so this needs no divide by w and interpolates exactly.
layout (location = 4) out vec3 out_shadowCoord;

// Light clip space ([-1, 1] in x and y) to shadow-map uv ([0, 1]); depth is already in [0, 1]
const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main()
{
    mat4 modelMatrix = uboInstanced.m_InstanceData[gl_InstanceIndex].m_ModelMatrix;

    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
    out_viewDirection = ubo.cameraWorldPosition - worldPosition.xyz;
    out_uv = in_uv;
	out_normal = (modelMatrix * vec4(in_normal, 0.0)).xyz;
    out_tangent = vec4((modelMatrix * vec4(in_tangent.xyz, 0.0)).xyz, in_tangent.w);

    // Normal offset: look the receiver up from a point moved off its surface, so a surface does not shadow
    // itself where the map's texels are larger than its pixels (shadow acne). The move grows as the surface
    // turns away from the sun, where the depth slope and so the acne are worst.
    vec3 N = normalize(out_normal);
    float NdotL = clamp(dot(N, normalize(lightingUbo.m_lighting.m_lightDirections[0].xyz)), 0.0, 1.0);
    vec3 offsetPosition = worldPosition.xyz + N * lightingUbo.m_lighting.m_shadowParams.x * (1.0 - NdotL);
    out_shadowCoord = (biasMat * lightingUbo.m_lighting.m_lightViewProjection * vec4(offsetPosition, 1.0)).xyz;
}