// PBR metallic-roughness material shared by the forward fragment shaders.
// Declares the per-material bindings (set 1 textures + material push constant),
// so include it after ShaderMaterial.h and metallic-roughness.glsl.

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D metallicTexture;
layout(set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D metallicRoughnessTexture;

layout (push_constant, std430) uniform PushFragment
{
    MaterialProperties m_materialProperties;
} pushConstants;

struct PbrSurface
{
    vec3 albedo;
    float alpha;
    vec3 N;
    float metallic;
    float roughness;
    vec3 F0;
};

// Samples the material at uv. Applies the glTF alpha mode, so MASK materials discard here.
PbrSurface getPbrSurface(vec2 uv, vec3 normal, vec3 tangent)
{
    PbrSurface surface;

    // color
    vec4 col;
    if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_DIFFUSE_MAP))
    {
        col = texture(baseColorTexture, uv) * pushConstants.m_materialProperties.m_baseColor;
    }
    else
    {
        col = pushConstants.m_materialProperties.m_baseColor;
    }
    surface.albedo = col.rgb;
    surface.alpha = col.a;

    // glTF alpha modes: OPAQUE ignores alpha, MASK alpha-tests against the cutoff
    if (bool(pushConstants.m_materialProperties.m_features & GLSL_ALPHA_MODE_MASK))
    {
        if (surface.alpha < pushConstants.m_materialProperties.m_AlphaCutoff)
            discard;
        surface.alpha = 1.0;
    }
    else if (!bool(pushConstants.m_materialProperties.m_features & GLSL_ALPHA_MODE_BLEND))
    {
        surface.alpha = 1.0;
    }

    vec3 texNormal = texture(normalTexture, uv).xyz * 2.0 - 1.0;
    surface.N = (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_NORMAL_MAP)) ? getNormalFromNormalMaps(texNormal, normal, tangent) : normalize(normal);

    surface.metallic = pushConstants.m_materialProperties.m_metallic;
    surface.roughness = pushConstants.m_materialProperties.m_roughness;

    if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_ROUGHNESS_METALLIC_MAP))
    {
        vec2 mr = texture(metallicRoughnessTexture, uv).bg;
        surface.metallic = mr.x;
        surface.roughness = mr.y;
    }
    else
    {
        if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_METALLIC_MAP))
        {
            surface.metallic = texture(metallicTexture, uv).r;
        }
        if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_ROUGHNESS_MAP))
        {
            surface.roughness = texture(roughnessTexture, uv).r;
        }
    }

    surface.F0 = mix(vec3(0.04), surface.albedo, surface.metallic);

    return surface;
}

// Diffuse + specular contribution of one directional light. L points towards the light.
vec3 pbrDirectLight(PbrSurface surface, vec3 V, vec3 L, vec3 radiance)
{
    vec3 N = surface.N;
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    float D = distributionGGX(N, H, surface.roughness);
    float G = geometrySmith(N, V, L, surface.roughness);
    vec3 F = fresnelSchlick(LdotH, surface.F0);

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL;
    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - surface.metallic;

    return (kD * surface.albedo + specular) * radiance * NdotL;
}

vec3 pbrAmbient(PbrSurface surface)
{
    return surface.albedo * 0.03;
}
