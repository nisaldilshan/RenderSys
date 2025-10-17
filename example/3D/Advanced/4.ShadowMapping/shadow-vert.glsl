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

mat4 createOrthoMatrixVulkan_NoYFlip(float left, float right, float bottom, float top, float near, float far) {
    float r_l = right - left;
    float t_b = top - bottom;
    float f_n = far - near;

    return mat4(
        vec4(2.0 / r_l, 0.0, 0.0, 0.0),                      // Column 1
        vec4(0.0, 2.0 / t_b, 0.0, 0.0),                      // Column 2 (Y is NOT flipped: +2.0)
        vec4(0.0, 0.0, -1.0 / f_n, 0.0),                     // Column 3 (Depth mapped to [0, 1])
        vec4(-(right + left) / r_l, -(top + bottom) / t_b, -near / f_n, 1.0) // Column 4
    );
}

void main()
{
    mat4 modelMatrix = uboInstanced.m_InstanceData[gl_InstanceIndex].m_ModelMatrix;

    // 1. Define the dimensions of your light's view volume.
    //    These values should be large enough to contain your entire scene
    //    from the light's point of view.
    const float orthoLeft   = -50.0;
    const float orthoRight  =  50.0;
    const float orthoBottom = -50.0;
    const float orthoTop    =  50.0;
    const float nearPlane   =   0.1;
    const float farPlane    = 200.0;

    // 2. Create the orthographic projection matrix.
    mat4 lightProjectionMatrix = createOrthoMatrixVulkan_NoYFlip(orthoLeft, orthoRight, orthoBottom, orthoTop, nearPlane, farPlane);

    vec4 worldPosition = modelMatrix * vec4(aPos, 1.0);
    gl_Position = lightProjectionMatrix  * lightingUbo.viewProjection[0] * worldPosition;
}