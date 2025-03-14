#version 460

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    float time;
    float _pad[3];
} ubo;

layout(set = 0, binding = 1) uniform LightingUniforms {
    vec4 directions[2];
    vec4 colors[2];
} lightingUbo;

struct Material {
    vec4 color;
    float hardness;
    float kd;
    float ks;
    float workflow;
    float metallic;
    float roughness;
    int colorTextureSet;
    int PhysicalDescriptorTextureSet;
    int normalTextureSet;
    float _pad;
};

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

const float PI = 3.14159265359;

// Function to calculate the Distribution of Microfacets (GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

// Function to calculate the Geometry Schlick-GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// Function to calculate Geometry Smith
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Function to calculate the Fresnel effect (Schlick's approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 getNormalFromNormalMaps()
{
	vec3 texNormal = texture(normalTexture, in_uv).xyz * 2.0 - 1.0;
	vec3 N = normalize(in_normal);
	vec3 T = normalize(in_tangent);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * texNormal);
}

void main()
{
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);
    vec3 texColor = texture(baseColorTexture, in_uv).rgb; 
    Material material = materialUbo.materials[pushConstants.materialIndex]; 
    N = (material.normalTextureSet > -1) ? getNormalFromNormalMaps() : N;
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