#version 460

#include "ShaderMaterial.h"
#include "metallic-roughness.glsl"

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    float time;
} ubo;

layout(std140, set = 0, binding = 1) uniform LightingUniforms {
    vec4 directions[1];
    vec4 colors[1];
    mat4 padding[1];
    mat4 viewProjection[1];
} lightingUbo;

layout(set = 0, binding = 2) uniform sampler2D shadowMap;

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D metallicTexture;
layout(set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D metallicRoughnessTexture;

layout (push_constant, std430) uniform PushFragment
{
    MaterialProperties m_materialProperties;
} pushConstants;

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;
layout (location = 4) in vec4 inShadowCoord;

layout (location = 0) out vec4 out_color;

#define ambientForShadow 0.1

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = ambientForShadow;
		}
	}
	return shadow;
}

void main()
{
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);

    // color
    vec4 col;
    float alpha = 1.0; // Default to opaque
    if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_DIFFUSE_MAP))
    {
        col = texture(baseColorTexture, in_uv) * pushConstants.m_materialProperties.m_baseColor;
        alpha = col.a; // <-- Store the alpha here
    }
    else
    {
        col = pushConstants.m_materialProperties.m_baseColor;
        alpha = col.a; // <-- Store the alpha here
    }

    vec3 texNormal = texture(normalTexture, in_uv).xyz * 2.0 - 1.0;
    N = (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_NORMAL_MAP)) ? getNormalFromNormalMaps(texNormal, in_normal, in_tangent) : N;
    vec3 albedo = col.rgb;

    float metallic = pushConstants.m_materialProperties.m_metallic;
    float roughness = pushConstants.m_materialProperties.m_roughness;

    if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_ROUGHNESS_METALLIC_MAP))
    {
        vec2 mr = texture(metallicRoughnessTexture, in_uv).bg;
        metallic = mr.x;
        roughness = mr.y;
    }
    else
    {
        if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_METALLIC_MAP))
        {
            metallic = texture(metallicTexture, in_uv).r;
        }
        if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_ROUGHNESS_MAP))
        {
            roughness = texture(roughnessTexture, in_uv).r;
        }
    }

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
        float D = distributionGGX(N, H, roughness);        
        float G = geometrySmith(N, V, L, roughness);      
        vec3 F = fresnelSchlick(LdotH, F0);  // F is now per-light 

        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular = numerator / max(denominator, 0.001);      

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
 
        vec3 radiance = lightingUbo.colors[i].rgb;
        total_diffuse += kD * radiance * albedo * NdotL;  // Use kD here
        total_specular += radiance * specular * NdotL;             
    }

    vec3 ambient = albedo * 0.03;

    vec3 color = (total_diffuse + total_specular + ambient); // No kD here

    float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
    out_color = vec4(color * shadow, alpha);
    out_color = pow(out_color, vec4(1.0/2.2));
}