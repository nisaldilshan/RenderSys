#version 460

#include "ShaderMaterial.h"
#include "metallic-roughness.glsl"
#include "pbr-material.glsl"

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

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec4 in_tangent;
layout (location = 4) in vec4 inShadowCoord;

layout (location = 0) out vec4 out_color;

#define ambientForShadow 0.1

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z - 0.005 ) 
		{
			shadow = ambientForShadow;
		}
	}
	return shadow;
}

// 1. Poisson Disk Constants (4 samples for efficiency, random rotation hides the low count)
const vec2 poissonDisk[4] = vec2[](
    vec2( -0.94201624, -0.39906216 ),
    vec2( 0.94558609, -0.76890725 ),
    vec2( -0.094184101, -0.92938870 ),
    vec2( 0.34495938, 0.29387760 )
);

// 2. Random Number Generator
float random(vec3 seed, int i){
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

// 3. The Main Filter Function
float filterPCF(vec4 sc)
{
    // Calculate texel size based on your scale factor
    ivec2 texDim = textureSize(shadowMap, 0);
    float scale = 1.5;
    vec2 texelSize = vec2(scale) / vec2(texDim);

    float shadowFactor = 0.0;
    
    // Create random rotation based on pixel coordinates to remove banding
    float noise = random(sc.xyz, 0); 
    float s = sin(noise * 6.28318530718);
    float c = cos(noise * 6.28318530718);
    mat2 rot = mat2(c, -s, s, c);

    // Loop through Poisson samples
    for (int i = 0; i < 4; i++)
    {
        // 1. Rotate the sample point
        vec2 offset = rot * poissonDisk[i];
        
        // 2. Scale it to texture coordinates (this matches your 'dx/dy' logic)
        offset *= texelSize;

        // 3. Call YOUR custom function with the calculated offset
        shadowFactor += textureProj(sc, offset);
    }
    
    return shadowFactor / 4.0;
}

void main()
{
    PbrSurface surface = getPbrSurface(in_uv, in_normal, in_tangent);
    vec3 V = normalize(in_viewDirection);

    vec3 color = pbrAmbient(surface);
    for (int i = 0; i < lightingUbo.directions.length(); ++i) 
    {
        color += pbrDirectLight(surface, V, normalize(lightingUbo.directions[i].xyz), lightingUbo.colors[i].rgb);
    }

    float shadow = filterPCF(inShadowCoord / inShadowCoord.w);
    out_color = vec4(color * shadow, surface.alpha);
    out_color = pow(out_color, vec4(1.0/2.2));
}