// Display transform for forward passes that write straight into the 8-bit UNORM image ImGui shows:
// exposure, a tone curve, then the sRGB encoding. Include it after ShaderLighting.h.

// Stephen Hill's fit of the ACES RRT and sRGB ODT (BakingLab, ACES.hlsl).
// GLSL matrices take columns, so these are the transposes of the HLSL rows.
const mat3 ACES_INPUT_MAT = mat3(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777);

const mat3 ACES_OUTPUT_MAT = mat3(
     1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602);

vec3 rrtAndOdtFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 tonemapAcesFitted(vec3 color)
{
    color = ACES_INPUT_MAT * color;
    color = rrtAndOdtFit(color);
    color = ACES_OUTPUT_MAT * color;
    return clamp(color, 0.0, 1.0);
}

// Khronos PBR Neutral, reference implementation from KhronosGroup/ToneMapping.
// Keeps base colors unchanged up to 0.76 when the radiance equals the albedo.
vec3 tonemapPbrNeutral(vec3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
        return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, vec3(newPeak), g);
}

// Exact sRGB OETF (IEC 61966-2-1), linear [0, 1] to encoded [0, 1]
vec3 linearToSrgb(vec3 color)
{
    color = clamp(color, 0.0, 1.0);
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lessThanEqual(color, vec3(0.0031308)));
}

// Linear scene values to the encoded values for an 8-bit UNORM target. With
// GLSL_DISPLAY_TRANSFORM_IN_POST set, returns them unchanged for a later pass to transform.
vec3 applyDisplayTransform(vec3 color, LightingProperties lighting)
{
    if (bool(lighting.m_settings.z & GLSL_DISPLAY_TRANSFORM_IN_POST))
        return color;

    color *= lighting.m_exposure.x;

    if (lighting.m_settings.y == GLSL_TONEMAP_ACES_FITTED)
        color = tonemapAcesFitted(color);
    else if (lighting.m_settings.y == GLSL_TONEMAP_PBR_NEUTRAL)
        color = tonemapPbrNeutral(color);

    return linearToSrgb(color);
}
