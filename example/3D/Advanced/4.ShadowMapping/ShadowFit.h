#pragma once

// Fits a directional light's orthographic shadow map, either to the whole scene or around the camera.
//
// Both fits are stable when the camera moves or turns, so shadow edges do not shimmer:
// - The light's view is a rotation only. It changes when the sun moves, never when the camera does.
// - FitSunShadowToBounds depends on the sun and the static casters alone.
// - FitSunShadowToSphere bounds the camera's view by a sphere, whose radius comes from the lens alone, so
//   it is the same every frame however the camera turns. The map's centre moves in whole texels, so each
//   texel covers the same patch of the world from one frame to the next.
// Each fit is a few hundred flops and allocates nothing.

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>

struct ShadowBounds
{
	glm::vec3 min{FLT_MAX};
	glm::vec3 max{-FLT_MAX};

	void Add(const glm::vec3& point)
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}
	bool IsValid() const { return min.x <= max.x; }
	glm::vec3 Corner(int index) const
	{
		return {(index & 1) ? max.x : min.x, (index & 2) ? max.y : min.y, (index & 4) ? max.z : min.z};
	}
};

struct ShadowSphere
{
	glm::vec3 center{0.0f};
	float radius = 0.0f;
};

// The smallest sphere around the part of a camera's view from nearDistance to farDistance, for a symmetric
// perspective lens with vertical field of view fovY. The radius depends only on the lens and the distances.
// Returns the sphere with its centre at the camera, moved along the view direction by centerDistance.
inline ShadowSphere FrustumSliceSphere(float fovY, float aspectRatio, float nearDistance, float farDistance, float& centerDistance)
{
	// The frustum's corner edges leave the view axis at slope k: k^2 = tan^2(fovY / 2) (1 + aspect^2)
	const float tanHalfY = std::tan(0.5f * fovY);
	const float k2 = tanHalfY * tanHalfY * (1.0f + aspectRatio * aspectRatio);
	const float n = nearDistance;
	const float f = farDistance;
	ShadowSphere sphere;
	if (k2 >= (f - n) / (f + n))
	{
		// A wide lens: the far rectangle's circumcircle already holds the near corners
		centerDistance = f;
		sphere.radius = f * std::sqrt(k2);
	}
	else
	{
		// The centre is equally far from the near and far corners
		centerDistance = 0.5f * (f + n) * (1.0f + k2);
		sphere.radius = 0.5f * std::sqrt((f - n) * (f - n) + 2.0f * (f * f + n * n) * k2 + (f + n) * (f + n) * k2 * k2);
	}
	return sphere;
}

struct SunShadowFit
{
	glm::mat4 viewProjection{1.0f}; // world to the light's clip space
	float texelWorldSize = 0.0f;    // the width of one shadow-map texel, in world units (the larger side if not square)
	float mapWorldSize = 0.0f;      // the width of the whole map, in world units (the larger side if not square)
};

namespace ShadowFitDetail
{
	// Right-handed, looking along the sunlight from the origin. Up is any axis away from the light's direction.
	inline glm::mat4 SunView(const glm::vec3& towardsSun)
	{
		const glm::vec3 lightForward = -towardsSun;
		const glm::vec3 up = std::abs(lightForward.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
		return glm::lookAtRH(glm::vec3(0.0f), lightForward, up);
	}

	// A little depth slack, so that geometry on the bounds does not land exactly on the near or far plane
	inline float DepthSlack(float nearestZ, float farthestZ) { return 0.01f * (nearestZ - farthestZ) + 1e-3f; }
}

// towardsSun is the unit vector from the scene to the sun. The map covers the casters' box as the sun sees it,
// as tightly as an axis-aligned box allows, and moves only when the sun does.
inline SunShadowFit FitSunShadowToBounds(const glm::vec3& towardsSun, const ShadowBounds& casters, uint32_t mapResolution)
{
	const glm::mat4 lightView = ShadowFitDetail::SunView(towardsSun);
	ShadowBounds lightSpace;
	for (int corner = 0; corner < 8; ++corner)
		lightSpace.Add(glm::vec3(lightView * glm::vec4(casters.Corner(corner), 1.0f)));

	SunShadowFit fit;
	const glm::vec3 size = lightSpace.max - lightSpace.min;
	fit.mapWorldSize = std::max(size.x, size.y);
	fit.texelWorldSize = fit.mapWorldSize / static_cast<float>(mapResolution);
	// The light looks down -z, so a larger z is nearer the sun
	const float slack = ShadowFitDetail::DepthSlack(lightSpace.max.z, lightSpace.min.z);
	const glm::mat4 lightProjection = glm::orthoRH_ZO(lightSpace.min.x, lightSpace.max.x, lightSpace.min.y, lightSpace.max.y,
													  -(lightSpace.max.z + slack), -(lightSpace.min.z - slack));
	fit.viewProjection = lightProjection * lightView;
	return fit;
}

// towardsSun is the unit vector from the scene to the sun. The map covers the receivers' sphere, and its depth
// range runs from the caster nearest the sun to the far side of the receivers, so casters outside the sphere
// still shadow it.
inline SunShadowFit FitSunShadowToSphere(const glm::vec3& towardsSun, const ShadowSphere& receivers, const ShadowBounds& casters,
										 uint32_t mapResolution, bool snapToTexels)
{
	const glm::mat4 lightView = ShadowFitDetail::SunView(towardsSun);

	SunShadowFit fit;
	fit.mapWorldSize = 2.0f * receivers.radius;
	fit.texelWorldSize = fit.mapWorldSize / static_cast<float>(mapResolution);

	glm::vec3 center = glm::vec3(lightView * glm::vec4(receivers.center, 1.0f));
	if (snapToTexels && fit.texelWorldSize > 0.0f)
	{
		// The map's edges are its centre plus or minus half a map, a whole number of texels, so snapping the
		// centre puts the edges and every texel boundary on the same world-space grid each frame
		center.x = std::floor(center.x / fit.texelWorldSize) * fit.texelWorldSize;
		center.y = std::floor(center.y / fit.texelWorldSize) * fit.texelWorldSize;
	}

	// The light looks down -z, so a larger z is nearer the sun
	float nearestZ = center.z + receivers.radius;
	float farthestZ = center.z - receivers.radius;
	if (casters.IsValid())
	{
		float castersNearestZ = -FLT_MAX;
		float castersFarthestZ = FLT_MAX;
		for (int corner = 0; corner < 8; ++corner)
		{
			const float z = glm::dot(glm::vec3(lightView[0][2], lightView[1][2], lightView[2][2]), casters.Corner(corner)) + lightView[3][2];
			castersNearestZ = std::max(castersNearestZ, z);
			castersFarthestZ = std::min(castersFarthestZ, z);
		}
		// Every caster between the sun and the receivers, and no depth spent on empty space past the scene
		nearestZ = std::max(nearestZ, castersNearestZ);
		farthestZ = std::max(farthestZ, castersFarthestZ);
	}
	const float slack = ShadowFitDetail::DepthSlack(nearestZ, farthestZ);

	const glm::mat4 lightProjection = glm::orthoRH_ZO(center.x - receivers.radius, center.x + receivers.radius,
													  center.y - receivers.radius, center.y + receivers.radius,
													  -(nearestZ + slack), -(farthestZ - slack));
	fit.viewProjection = lightProjection * lightView;
	return fit;
}
