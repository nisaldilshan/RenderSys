#version 460
layout(binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    vec4 color;
    vec3 cameraWorldPosition;
    float time;
} ubo;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_uv;

layout (location = 0) out vec3 out_viewDirection;

void main() 
{
    vec4 worldPosition = ubo.modelMatrix * vec4(aPos, 1.0);
    gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
    out_viewDirection = ubo.cameraWorldPosition - worldPosition.xyz;
}