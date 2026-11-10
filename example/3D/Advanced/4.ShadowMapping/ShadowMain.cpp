#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <Walnut/Application.h>
#include <RenderSys/Walnut/WalnutAdapter.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <RenderSys/Camera/EditorCameraController.h>
#include <RenderSys/Walnut/WalnutInput.h>
#include <RenderSys/Scene/Model.h>
#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/LightComponents.h>
#include <RenderSys/Scene/Scene.h>
#include <RenderSys/Scene/SceneHierarchyPanel.h>
#include <imgui.h>

#include <glm/gtc/constants.hpp>
#include <resources/Shaders/ShaderLighting.h>

#include "Common/ExampleDockLayout.h"
#include "Common/GpuTimingPanel.h"
#include "Common/SunSkyRig.h"
#include "ShadowFit.h"

struct alignas(16) MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

static_assert(sizeof(LightingProperties) == 208, "must match the std140 layout in ShaderLighting.h");

constexpr uint32_t kShadowMapResolution = 2048;

// The sun's shadow, tuned in the Settings window
struct SunShadowSettings
{
	enum class Fit : int
	{
		WholeScene = 0, // the map covers every caster; it moves only with the sun
		CameraView,     // the map covers the view up to the shadow distance, or the scene when that is smaller
	};

	Fit fit = Fit::WholeScene;
	float shadowDistance = 20.0f;   // how far from the camera the camera-view fit reaches, in world units
	bool texelSnapping = true;      // move the camera-view fit in whole texels, so edges do not shimmer
	float depthBiasConstant = 1.25f; // vkCmdSetDepthBias constant factor
	float depthBiasSlope = 1.75f;    // vkCmdSetDepthBias slope factor
	float normalOffsetTexels = 1.0f; // receivers move this many texels along their normal before the lookup
	int pcfRadius = 1;               // 0 is one hardware 2x2 tap, 1 is 3x3 taps, 2 is 5x5 taps
	// Sponza has single-sided walls and leaves, which leak light if the faces that face the sun are culled
	RenderSys::CullMode casterCulling = RenderSys::CullMode::None;
	int debugView = GLSL_SHADOW_DEBUG_NONE;
	bool showShadowMap = true;
};

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_unique<RenderSys::Renderer3D>();
		m_renderer->Init();
		m_renderer->SetClearColor(m_clearColor); // OnUIRender only applies changes, and starts equal to it

		if (!loadScene())
		{
			assert(false);
			return;
		}

		loadShaders();

		m_cameraController = std::make_unique<RenderSys::EditorCameraController>(30.0f, 0.01f, 500.0f);
		auto cameraEntity = m_scene->AddCamera(m_cameraController->GetCamera());
		m_cameraController->SetCameraEntity(cameraEntity, m_scene->m_Registry);
		// Start facing the top brick wall of the Sponza atrium, which the default sun lights at a grazing angle.
		// Its glTF tangents have w = +1, so its bricks look lit from above only if the shader honours tangent.w.
		auto& cameraTransform = m_scene->m_Registry.get<RenderSys::TransformComponent>(cameraEntity);
		cameraTransform.SetTranslation(glm::vec3(10.0f, 62.0f, -3.0f));
		cameraTransform.SetRotation(glm::vec3(0.0f, glm::pi<float>(), 0.0f));

		std::vector<RenderSys::VertexAttribute> vertexAttribs(5);

		// Position attribute
		vertexAttribs[0].location = 0;
		vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[0].offset = 0;

		// Normal attribute
		vertexAttribs[1].location = 1;
		vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[1].offset = offsetof(RenderSys::Vertex, normal);

		// UV attribute
		vertexAttribs[2].location = 2;
		vertexAttribs[2].format = RenderSys::VertexFormat::Float32x2;
		vertexAttribs[2].offset = offsetof(RenderSys::Vertex, texcoord0);

		// Color attribute
		vertexAttribs[3].location = 3;
		vertexAttribs[3].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[3].offset = offsetof(RenderSys::Vertex, color);

		// Tangent attribute
		vertexAttribs[4].location = 4;
		vertexAttribs[4].format = RenderSys::VertexFormat::Float32x4;
		vertexAttribs[4].offset = offsetof(RenderSys::Vertex, tangent);

		RenderSys::VertexBufferLayout vertexBufferLayout;
		vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
		vertexBufferLayout.attributes = vertexAttribs.data();
		vertexBufferLayout.arrayStride = sizeof(RenderSys::Vertex);
		vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

		auto view = m_scene->m_Registry.view<RenderSys::MeshComponent, RenderSys::TransformComponent>();
		for (auto entity : view)
		{
			auto& meshComponent = view.get<RenderSys::MeshComponent>(entity);
			auto vertexBuffer = meshComponent.m_Mesh->m_meshData->getVertexBufferForRenderer();
			if (vertexBuffer.size() == 5718) // woman model
				m_models[1].applyVertexSkinningOnCPU(vertexBuffer);
			assert(vertexBuffer.size() > 0);
			const auto vertexBufID = m_renderer->SetVertexBufferData(vertexBuffer, vertexBufferLayout);
			meshComponent.m_Mesh->vertexBufferID = vertexBufID;
			assert(meshComponent.m_Mesh->m_meshData->indices.size() > 0);
			m_renderer->SetIndexBufferData(vertexBufID, meshComponent.m_Mesh->m_meshData->indices);
		}

		m_scene->AddInstanceOfSubTree(0, glm::vec3(0.0f, 0.0f, 0.0f), m_scene->m_rootNodeIndex, m_scene->m_instancedRootNodeIndex);
		m_scene->AddDirectionalLight(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)); // aimLightAtSun aims it every frame

		std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(3);
		// The uniform buffer binding that we already had
		RenderSys::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
		uniformBindingLayout.setDefault();
		uniformBindingLayout.binding = 0;
		uniformBindingLayout.visibility = RenderSys::ShaderStage::VertexAndFragment;
		uniformBindingLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
		uniformBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
		uniformBindingLayout.buffer.hasDynamicOffset = false;
		// Lighting Uniforms
		RenderSys::BindGroupLayoutEntry& lightingUniformLayout = bindingLayoutEntries[1];
		lightingUniformLayout.setDefault();
		lightingUniformLayout.binding = 1;
		lightingUniformLayout.visibility = RenderSys::ShaderStage::VertexAndFragment;
		lightingUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
		lightingUniformLayout.buffer.minBindingSize = sizeof(LightingProperties);
		// ShadowMap binding, a sampler2DShadow
		RenderSys::BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[2];
		textureBindingLayout.setDefault();
		textureBindingLayout.binding = 2;
		textureBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
		textureBindingLayout.texture.sampleType = RenderSys::TextureSampleType::Float;
		textureBindingLayout.texture.viewDimension = RenderSys::TextureViewDimension::_2D;

		m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
		m_renderer->CreateUniformBuffer(lightingUniformLayout.binding, sizeof(LightingProperties), 1);

		m_renderer->CreateShadowMap(kShadowMapResolution, kShadowMapResolution);
		m_renderer->CreateBindGroup(bindingLayoutEntries);
		m_renderer->CreatePipeline();
		m_renderer->CreateShadowPipeline(m_shadow.casterCulling);
	}

	virtual void OnDetach() override
	{
		m_models.clear();
		m_sceneHierarchyPanel.reset();
		m_scene.reset();

		m_renderer->Destroy();
	}

	virtual void OnUpdate(float ts) override
	{
        Walnut::Timer timer;
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

		if (m_viewportWidth != m_renderer->GetWidth() ||
			m_viewportHeight != m_renderer->GetHeight())
		{
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);
			m_cameraController->GetCamera()->SetAspectRatio(static_cast<float>(m_viewportWidth)/ static_cast<float>(m_viewportHeight));
		}

		if (m_renderer)
		{
			m_cameraController->OnUpdate(RenderSys::GetCameraInputFromWalnut());
			if (m_moveLightDirection)
				moveLight(ts);
			aimLightAtSun();
			m_scene->Update();
			if (!m_sceneBoundsComputed)
			{
				computeSceneBounds(); // needs the instance matrices that the first Update writes
				m_sceneBoundsComputed = true;
			}

			updateUniforms();
			m_renderer->SetShadowDepthBias(m_shadow.depthBiasConstant, m_shadow.depthBiasSlope);

			m_renderer->BeginFrame();

			m_renderer->ShadowPass(m_scene->m_Registry);

			m_renderer->BeginRenderPass();
			m_renderer->BindResources();
			auto meshView = m_scene->m_Registry.view<RenderSys::MeshComponent,
												RenderSys::TransformComponent,
												RenderSys::InstanceTagComponent>();
			for (auto entity : meshView)
			{
				auto& meshComponent = meshView.get<RenderSys::MeshComponent>(entity);
				m_renderer->RenderMesh(meshComponent);
			}

			m_renderer->EndRenderPass();
			m_renderer->EndFrame();
		}

        m_lastRenderTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", m_lastRenderTime);
		m_gpuTimingPanel.OnImGuiRender(*m_renderer);
		static ImVec4 newClearColorImgui = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGui::ColorEdit3("Clear Color", (float*)&newClearColorImgui);
		glm::vec4 newClearColor = {newClearColorImgui.x, newClearColorImgui.y, newClearColorImgui.z, newClearColorImgui.w};
		if (newClearColor != m_clearColor)
		{
			m_clearColor = newClearColor;
			m_renderer->SetClearColor(m_clearColor);
		}
		ImGui::Checkbox("Move Light Direction", &m_moveLightDirection);
		ImGui::SetItemTooltip("Sweeps the sun's azimuth at 10 degrees a second");
		if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
			shadowSettingsUI();
		if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
			m_lightRig.OnImGuiRender();
		ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
        m_viewportHeight = ImGui::GetContentRegionAvail().y;
		const float imageWidth = m_renderer->GetWidth();
		const float imageHeight = m_renderer->GetHeight();
        ImGui::Image(m_renderer->GetDescriptorSet(), {imageWidth, imageHeight});
		ImGui::End();

		if (m_shadow.showShadowMap)
		{
			ImGui::Begin("Shadow Map", &m_shadow.showShadowMap);
			const uint64_t shadowMapTexture = m_renderer->GetShadowMapImGuiTexture();
			if (shadowMapTexture != 0)
			{
				// Square, as large as fits. Depth is greyscale, black nearest the sun. The map's first row is the
				// light's bottom edge, so flip it to show the light's up as up.
				const ImVec2 available = ImGui::GetContentRegionAvail();
				const float side = std::max(1.0f, std::min(available.x, available.y));
				ImGui::Image((ImTextureID)shadowMapTexture, {side, side}, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
			}
			ImGui::End();
		}
        ImGui::PopStyleVar();

		m_sceneHierarchyPanel->OnImGuiRender();
	}

private:
	void shadowSettingsUI()
	{
		const char* fits[] = {"Whole scene", "Camera view"};
		int fit = static_cast<int>(m_shadow.fit);
		if (ImGui::Combo("Fit", &fit, fits, IM_ARRAYSIZE(fits)))
			m_shadow.fit = static_cast<SunShadowSettings::Fit>(fit);
		ImGui::SetItemTooltip("Whole scene: the map covers every caster and moves only with the sun.\n"
							  "Camera view: the map covers what the camera sees up to the shadow distance,\n"
							  "for sharper shadows near the camera.");
		if (m_shadow.fit == SunShadowSettings::Fit::CameraView)
		{
			ImGui::SliderFloat("Shadow distance", &m_shadow.shadowDistance, 1.0f, 500.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
			ImGui::Checkbox("Texel snapping", &m_shadow.texelSnapping);
			ImGui::SetItemTooltip("Moves the map in whole texels. Turn it off and move the camera to see edges shimmer.");
			if (!m_fitFollowsCamera)
				ImGui::TextDisabled("The whole-scene map is sharper at this distance, so it is used");
		}
		ImGui::Text("Map %.1f units wide, texel %.3f units", m_shadowFit.mapWorldSize, m_shadowFit.texelWorldSize);
		ImGui::Text("Fit on the CPU: %.2f us", m_fitMicroseconds);

		ImGui::SliderFloat("Depth bias", &m_shadow.depthBiasConstant, 0.0f, 20.0f, "%.2f");
		ImGui::SetItemTooltip("Constant depth bias, in the depth format's smallest steps");
		ImGui::SliderFloat("Slope bias", &m_shadow.depthBiasSlope, 0.0f, 10.0f, "%.2f");
		ImGui::SetItemTooltip("Depth bias per unit of a caster's depth slope; the main cure for acne on tilted surfaces");
		ImGui::SliderFloat("Normal offset", &m_shadow.normalOffsetTexels, 0.0f, 4.0f, "%.2f texels");
		ImGui::SetItemTooltip("Moves each receiver along its normal before the lookup, more where it faces away from the sun");
		const char* pcfKernels[] = {"2x2 (one hardware tap)", "3x3 taps", "5x5 taps"};
		ImGui::Combo("PCF", &m_shadow.pcfRadius, pcfKernels, IM_ARRAYSIZE(pcfKernels));
		const char* cullModes[] = {"None", "Faces towards the sun", "Faces away from the sun"};
		int casterCulling = static_cast<int>(m_shadow.casterCulling);
		if (ImGui::Combo("Cull casters", &casterCulling, cullModes, IM_ARRAYSIZE(cullModes)))
		{
			m_shadow.casterCulling = static_cast<RenderSys::CullMode>(casterCulling);
			m_renderer->CreateShadowPipeline(m_shadow.casterCulling);
		}
		ImGui::SetItemTooltip("Culling the faces towards the sun stops acne on closed meshes, but single-sided\n"
							  "geometry such as Sponza's walls and leaves then lets light through");

		const char* debugViews[] = {"Off", "Sun visibility", "Shadow-map texels"};
		ImGui::Combo("Debug view", &m_shadow.debugView, debugViews, IM_ARRAYSIZE(debugViews));
		ImGui::SetItemTooltip("Sun visibility: white where the sun reaches, black in shadow.\n"
							  "Shadow-map texels: a checkerboard of the map's texels, still on surfaces when snapping works.\n"
							  "Both tint red what the map does not cover.");
		ImGui::Checkbox("Show shadow map", &m_shadow.showShadowMap);
	}

	void updateUniforms()
	{
		auto camera = m_cameraController->GetCamera();
		m_myUniformData.viewMatrix = camera->GetViewMatrix();
		m_myUniformData.projectionMatrix = camera->GetProjectionMatrix();
		m_myUniformData.cameraWorldPosition = camera->GetPosition();
		m_myUniformData.time = 0.0f;
		m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);

		// The sun is light 0, and the shadow map is rendered from it
		m_lightingUniformData = m_lightRig.ToLightingProperties();

		const auto fitStart = std::chrono::steady_clock::now();
		const glm::vec3 towardsSun = m_lightRig.TowardsSun();
		m_fitFollowsCamera = false;
		if (m_sceneBounds.IsValid())
			m_shadowFit = FitSunShadowToBounds(towardsSun, m_sceneBounds, kShadowMapResolution);
		if (m_shadow.fit == SunShadowSettings::Fit::CameraView || !m_sceneBounds.IsValid())
		{
			const float farDistance = std::clamp(m_shadow.shadowDistance, 2.0f * camera->GetNearClip(), camera->GetFarClip());
			float centerDistance = 0.0f;
			ShadowSphere cameraSphere = FrustumSliceSphere(glm::radians(camera->GetFOV()), camera->GetAspectRatio(),
															camera->GetNearClip(), farDistance, centerDistance);
			cameraSphere.center = camera->GetPosition() + centerDistance * camera->GetForwardDirection();
			const SunShadowFit cameraFit = FitSunShadowToSphere(towardsSun, cameraSphere, m_sceneBounds, kShadowMapResolution,
																m_shadow.texelSnapping);
			// Once the view reaches about the scene's size, the whole-scene map is the sharper one
			if (!m_sceneBounds.IsValid() || cameraFit.texelWorldSize < m_shadowFit.texelWorldSize)
			{
				m_shadowFit = cameraFit;
				m_fitFollowsCamera = true;
			}
		}
		const auto fitEnd = std::chrono::steady_clock::now();
		// A running average, since one fit is close to the clock's resolution
		const float fitMicroseconds = std::chrono::duration<float, std::micro>(fitEnd - fitStart).count();
		m_fitMicroseconds += 0.05f * (fitMicroseconds - m_fitMicroseconds);

		m_lightingUniformData.m_lightViewProjection = m_shadowFit.viewProjection;
		m_lightingUniformData.m_settings.w = m_shadow.debugView;
		m_lightingUniformData.m_shadowParams = glm::vec4(m_shadow.normalOffsetTexels * m_shadowFit.texelWorldSize,
														 static_cast<float>(m_shadow.pcfRadius), 0.0f, 0.0f);
		m_renderer->SetUniformBufferData(1, &m_lightingUniformData, 0);
	}

	// The world bounds of every mesh instance, which are the shadow casters. The scene is static, so this runs once.
	void computeSceneBounds()
	{
		m_sceneBounds = ShadowBounds{};
		auto meshView = m_scene->m_Registry.view<RenderSys::MeshComponent, RenderSys::InstanceTagComponent>();
		for (auto entity : meshView)
		{
			const auto& meshComponent = meshView.get<RenderSys::MeshComponent>(entity);
			const auto& instanceTag = meshView.get<RenderSys::InstanceTagComponent>(entity);
			ShadowBounds meshBounds;
			for (const auto& vertex : meshComponent.m_Mesh->m_meshData->vertices)
				meshBounds.Add(vertex.pos);
			if (!meshBounds.IsValid())
				continue;

			const auto instanceBuffer = instanceTag.GetInstanceBuffer();
			for (uint32_t instance = 0; instance < instanceTag.GetInstanceCount(); ++instance)
			{
				const glm::mat4& modelMatrix = instanceBuffer->GetModelMatrix(meshComponent.m_FirstInstance + instance);
				for (int corner = 0; corner < 8; ++corner)
					m_sceneBounds.Add(glm::vec3(modelMatrix * glm::vec4(meshBounds.Corner(corner), 1.0f)));
			}
		}
		std::cout << "Shadow casters span (" << m_sceneBounds.min.x << ", " << m_sceneBounds.min.y << ", " << m_sceneBounds.min.z
				  << ") to (" << m_sceneBounds.max.x << ", " << m_sceneBounds.max.y << ", " << m_sceneBounds.max.z << ")" << std::endl;
	}

	// Sweeps the sun around the sky at 10 degrees a second, which turns every shadow
	void moveLight(float ts)
	{
		m_lightRig.sunAzimuthDeg += 10.0f * ts;
		if (m_lightRig.sunAzimuthDeg > 180.0f)
			m_lightRig.sunAzimuthDeg -= 360.0f;
	}

	// The scene's directional light follows the rig, so the Properties panel matches the lighting. The shadow fit
	// reads the rig directly. TransformComponent's forward is quat(euler) * +z, which for euler (pitch, yaw, 0) is
	// (cos pitch sin yaw, -sin pitch, cos pitch cos yaw): the elevation and azimuth as pitch and yaw point
	// it along the sunlight, away from TowardsSun().
	void aimLightAtSun()
	{
		auto lightView = m_scene->m_Registry.view<RenderSys::DirectionalLightComponent,
											RenderSys::TransformComponent>();
		for (auto entity : lightView)
		{
			auto& lightComponent = lightView.get<RenderSys::DirectionalLightComponent>(entity);
			lightComponent.m_Color = m_lightRig.sunColor;
			lightComponent.m_LightIntensity = m_lightRig.sunIntensity;

			auto& transformComponent = lightView.get<RenderSys::TransformComponent>(entity);
			transformComponent.SetRotation(glm::vec3(glm::radians(m_lightRig.sunElevationDeg), glm::radians(m_lightRig.sunAzimuthDeg), 0.0f));
		}
	}

	bool loadScene()
	{
		m_scene = std::make_shared<RenderSys::Scene>();
		m_models.reserve(2);
		auto& model1 = m_models.emplace_back(*m_scene);
		if (!model1.load(RESOURCE_DIR "/Models/Sponza/glTF/Sponza.gltf"))
		{
			std::cout << "Error loading GLTF model!" << std::endl;
			return false;
		}

		auto& model2 = m_models.emplace_back(*m_scene);
		if (!model2.load(RESOURCE_DIR "/Models/Woman.gltf"))
		{
			std::cout << "Error loading GLTF model!" << std::endl;
			return false;
		}

		m_scene->printNodeGraph();
		m_sceneHierarchyPanel = std::make_unique<RenderSys::SceneHierarchyPanel>(m_scene);

		return true;
	}

	void loadShader(const std::string& fileName, const std::string& shaderName, RenderSys::ShaderStage stage)
	{
		const auto shaderDir = std::filesystem::path(SHADER_DIR).string();
		assert(!shaderDir.empty());

		std::ifstream file(shaderDir + "/" + fileName, std::ios::binary);
		if (!file.is_open()) {
			std::cerr << "Unable to open file " << fileName << std::endl;
			assert(false);
			return;
		}
		std::vector<char> content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		RenderSys::Shader shader(shaderName, std::string(content.data(), content.size()));
		shader.type = RenderSys::ShaderType::SPIRV;
		shader.stage = stage;
		shader.SetIncludeDirectory(std::string(RESOURCE_DIR) + "/Shaders");
		m_renderer->SetShader(shader);
	}

	void loadShaders()
	{
		if (Walnut::RenderingBackend::GetBackend() != Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			assert(false);
			return;
		}
		loadShader("ShadowMain-vert.glsl", "Vertex", RenderSys::ShaderStage::Vertex);
		loadShader("ShadowMain-frag.glsl", "Fragment", RenderSys::ShaderStage::Fragment);
		loadShader("shadow-vert.glsl", "Shadow-Vertex", RenderSys::ShaderStage::Vertex);
		loadShader("shadow-frag.glsl", "Shadow-Fragment", RenderSys::ShaderStage::Fragment);
	}

    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
	GpuTimingPanel m_gpuTimingPanel;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	glm::vec4 m_clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyUniforms m_myUniformData;
	SunSkyRig m_lightRig;
	LightingProperties m_lightingUniformData;
	std::unique_ptr<RenderSys::EditorCameraController> m_cameraController;
	std::shared_ptr<RenderSys::Scene> m_scene;
	std::vector<RenderSys::Model> m_models;
	std::unique_ptr<RenderSys::SceneHierarchyPanel> m_sceneHierarchyPanel;
	bool m_moveLightDirection = false;

	SunShadowSettings m_shadow;
	ShadowBounds m_sceneBounds;   // the casters, in world space
	bool m_sceneBoundsComputed = false;
	SunShadowFit m_shadowFit;     // this frame's map placement
	bool m_fitFollowsCamera = false;
	float m_fitMicroseconds = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";
#ifndef NDEBUG
	spec.EnableGraphicsValidation = true; // used only when the loader finds the layer, see VK_LAYER_PATH in launch.json
#endif

	Walnut::Application* app = new Walnut::Application(spec);
	RenderSys::AttachToWalnut();
	ExampleDockLayout layout;
	layout.Left = { "Scene Hierarchy", "Properties", "Shadow Map" };
	layout.SetAsDefault(*app);
	app->PushLayer<Renderer3DLayer>();
	return app;
}
