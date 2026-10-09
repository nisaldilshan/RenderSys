#version 460

#include "ShaderMaterial.h"

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;

layout (push_constant, std430) uniform PushFragment
{
    MaterialProperties m_materialProperties;
} pushConstants;

layout (location = 0) in vec2 in_uv;

void main() 
{
	// alpha-masked materials (e.g. foliage) must not cast shadows from their transparent texels
	if (bool(pushConstants.m_materialProperties.m_features & GLSL_ALPHA_MODE_MASK))
	{
		float alpha = pushConstants.m_materialProperties.m_baseColor.a;
		if (bool(pushConstants.m_materialProperties.m_features & GLSL_HAS_DIFFUSE_MAP))
			alpha *= texture(baseColorTexture, in_uv).a;
		if (alpha < pushConstants.m_materialProperties.m_AlphaCutoff)
			discard;
	}
}
