#version 460

#include "shadermaterial.glsl"
#include "metallic-roughness.glsl"

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

layout(set = 1, binding = 0) uniform MaterialUniforms {
    Material materials[32];
} materialUbo;

layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;

layout (push_constant) uniform PushConstants {
    int materialIndex;
} pushConstants;

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

layout (location = 0) out vec4 out_color;

void main()
{
    Material material = materialUbo.materials[pushConstants.materialIndex];

    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);
    vec3 texColor = texture(baseColorTexture, in_uv).rgb;
    vec3 albedo = texColor * material.color.rgb;

    float metallic = material.metallic;
    float roughness = material.roughness;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 total_diffuse = vec3(0.0);
    vec3 total_specular = vec3(0.0);
    for (int i = 0; i < 2; ++i) 
    {
        vec3 L = normalize(lightingUbo.directions[i].xyz);
        vec3 H = normalize(V + L);

        float NdotV = max(dot(N, V), 0.0);
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float LdotH = max(dot(L, H), 0.0);

        // Cook-Torrance BRDF
        float D = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(LdotH, F0);  // F is now per-light

        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.001;
        vec3 specular = numerator / denominator;

        // --- Calculate kS and kD *inside* the loop ---
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        vec3 radiance = lightingUbo.colors[i].rgb;
        total_diffuse += kD * radiance * albedo * NdotL;  // Use kD here
        total_specular += radiance * specular * NdotL;
    }

    vec3 ambient = albedo * 0.03;

    vec3 color = (total_diffuse + total_specular + ambient); // No kD here

    out_color = vec4(color, 1.0);
    out_color = pow(out_color, vec4(1.0/2.2));
}