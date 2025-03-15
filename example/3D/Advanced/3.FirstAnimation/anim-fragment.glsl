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
layout(set = 1, binding = 2) uniform sampler2D normalTexture;

layout (push_constant) uniform PushConstants {
    int materialIndex;
} pushConstants;

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);
    vec3 texColor = texture(baseColorTexture, in_uv).rgb; 
    Material material = materialUbo.materials[pushConstants.materialIndex]; 
    vec3 texNormal = texture(normalTexture, in_uv).xyz * 2.0 - 1.0;
    N = (material.normalTextureSet > -1) ? getNormalFromNormalMaps(texNormal, in_normal, in_tangent) : N;
    vec3 materialColor = texColor * material.color.rgb;

    vec3 color;
    if (material.workflow == 1.0f) // specular glossiness 
    {
        vec3 diffuse = vec3(0.0);
        float specular = 0.0;
        for(int i = 0; i < 2; ++i)
        {
            vec3 lightColor = lightingUbo.colors[i].rgb;
            vec3 L = normalize(lightingUbo.directions[i].xyz);
            vec3 R = reflect(-L, N);
            diffuse += max(0.0, dot(L, N)) * lightColor;
            float RoV = max(0.0, dot(R, V));
            specular += pow(RoV, materialUbo.materials[pushConstants.materialIndex].hardness);
        }

        vec3 ambient = vec3(0.05);
        color = material.kd * diffuse + material.ks * specular + ambient;
        color = color * materialColor;
    }
    else // metallic-roughness
    {
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, materialColor, material.metallic);
        vec3 Lo = vec3(0.0); 
        for(int i = 0; i < 2; ++i) 
        {
            vec3 lightColor = lightingUbo.colors[i].rgb;
            vec3 L = normalize(lightingUbo.directions[i].xyz);
            vec3 H = normalize(V + L);
            float NDF = DistributionGGX(N, H, material.roughness);        
            float G   = GeometrySmith(N, V, L, material.roughness);      
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - material.metallic;	  

            vec3 numerator    = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
            vec3 specular     = numerator / max(denominator, 0.001);  

            float NdotL = max(dot(N, L), 0.0);  
            Lo += (kD * materialColor / PI + specular) * lightColor * NdotL;              
        }

        vec3 ambient = vec3(0.03) * texColor;
        color = Lo;
    }

    out_color = vec4(color, 1.0);
}