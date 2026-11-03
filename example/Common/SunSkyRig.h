#pragma once

#include <resources/Shaders/ShaderLighting.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>

#include <cmath>

// One sun plus a sky-and-ground ambient term, like the sun and sky light of a UE level.
// The defaults match the rr_oks UE5 level: the sun comes from its DirectionalLight (pitch -49.5,
// yaw -10.3), mapped to glTF as (x, z, -y) like the OKS converter, assuming the UE map keeps the
// OKS axes. The sky and ground values are fitted to its shaded walls. Tune in the Settings window.
struct SunSkyRig
{
	float sunElevationDeg = 50.0f; // above the horizon
	float sunAzimuthDeg = 100.0f;  // from -z towards -x
	glm::vec3 sunColor = {1.0f, 0.93f, 0.84f};
	float sunIntensity = 1.0f;     // 1 lights a white surface facing the sun to radiance 1
	glm::vec3 skyColor = {0.92f, 0.95f, 1.0f};
	float skyToSun = 0.24f;        // sky irradiance on an upward face over sun illuminance, pi * L_sky / E_sun
	glm::vec3 groundColor = {1.0f, 0.96f, 0.90f};
	float groundToSun = 0.08f;     // the same ratio for light from below
	float exposureEV = 1.0f;
	int toneMapper = GLSL_TONEMAP_ACES_FITTED;

	// Unit vector from the scene towards the sun
	glm::vec3 TowardsSun() const
	{
		const float elevation = glm::radians(sunElevationDeg);
		const float azimuth = glm::radians(sunAzimuthDeg);
		return {-std::cos(elevation) * std::sin(azimuth),
				std::sin(elevation),
				-std::cos(elevation) * std::cos(azimuth)};
	}

	// Light 0 is the sun. m_lightViewProjection is left at identity for the caller to fill in.
	LightingProperties ToLightingProperties() const
	{
		LightingProperties lighting{};
		lighting.m_lightDirections[0] = glm::vec4(TowardsSun(), 0.0f);
		// Illuminance pi gives radiance 1 off a white Lambert surface (BRDF 1/pi) facing the sun
		lighting.m_lightColors[0] = glm::vec4(glm::pi<float>() * sunIntensity * sunColor, 0.0f);
		// A uniform sky of radiance L gives irradiance pi * L on an upward face,
		// so L = ratio * E_sun / pi = ratio * sunIntensity
		lighting.m_skyColor = glm::vec4(skyToSun * sunIntensity * skyColor, 0.0f);
		lighting.m_groundColor = glm::vec4(groundToSun * sunIntensity * groundColor, 0.0f);
		lighting.m_lightViewProjection = glm::mat4(1.0f);
		lighting.m_exposure = glm::vec4(std::exp2(exposureEV), 0.0f, 0.0f, 0.0f);
		lighting.m_settings = glm::ivec4(1, toneMapper, 0, 0);
		return lighting;
	}

	void OnImGuiRender()
	{
		ImGui::SliderFloat("Sun elevation", &sunElevationDeg, 0.0f, 90.0f, "%.0f deg");
		ImGui::SliderFloat("Sun azimuth", &sunAzimuthDeg, -180.0f, 180.0f, "%.0f deg");
		ImGui::SetItemTooltip("Measured from -z towards -x");
		ImGui::ColorEdit3("Sun color", &sunColor.x);
		ImGui::SliderFloat("Sun intensity", &sunIntensity, 0.0f, 4.0f);
		ImGui::ColorEdit3("Sky color", &skyColor.x);
		ImGui::SliderFloat("Sky / sun", &skyToSun, 0.0f, 1.0f);
		ImGui::SetItemTooltip("Sky light on an upward face relative to the sun, pi * L_sky / E_sun");
		ImGui::ColorEdit3("Ground color", &groundColor.x);
		ImGui::SliderFloat("Ground / sun", &groundToSun, 0.0f, 1.0f);
		const char* toneMappers[] = {"None (clamp)", "ACES fit", "Khronos PBR Neutral"};
		ImGui::Combo("Tone curve", &toneMapper, toneMappers, IM_ARRAYSIZE(toneMappers));
		ImGui::SliderFloat("Exposure", &exposureEV, -4.0f, 4.0f, "%+.1f EV");
		if (ImGui::Button("Reset lighting"))
			*this = SunSkyRig{};
	}
};
