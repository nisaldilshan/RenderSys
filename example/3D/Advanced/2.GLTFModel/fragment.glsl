#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    vec4 color;
    float time;
    float _pad[3];
} ubo;

layout(binding = 1) uniform texture2D tex;
layout(binding = 2) uniform sampler s;

layout(binding = 3) uniform LightingUniforms {
    vec4 directions[2];
    vec4 colors[2];
    float hardness;
    float kd;
    float ks;
    float _pad;
} lightingUbo;

layout (location = 0) in vec3 in_viewDirection;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 color = vec3(1.0, 1.0, 1.0);
    // Gamma-correction
    vec3 corrected_color = pow(color, vec3(2.2));
    out_color = vec4(corrected_color, 1.0);
}