#include <array>
#include <fstream>
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
#include <RenderSys/Scene/Scene.h>
#include <RenderSys/Scene/SceneHierarchyPanel.h>
#include <resources/Shaders/ShaderLighting.h>
#include <glm/gtc/constants.hpp>
#include <imgui.h>

struct alignas(16) MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);
static_assert(sizeof(LightingProperties) == 192, "must match the std140 layout in ShaderLighting.h");

// One sun plus a sky-and-ground ambient term, like the sun and sky light of a UE level.
// Values are tuned by eye and exposed in the Settings window.
struct SunSkyRig
{
	float sunElevationDeg = 50.0f; // above the horizon
	float sunAzimuthDeg = 35.0f;   // from -z towards -x
	glm::vec3 sunColor = {1.0f, 0.93f, 0.84f};
	float sunIntensity = 1.0f;     // 1 lights a white surface facing the sun to radiance 1
	glm::vec3 skyColor = {0.80f, 0.89f, 1.0f};
	float skyToSun = 0.18f;        // sky irradiance on an upward face over sun illuminance, pi * L_sky / E_sun
	glm::vec3 groundColor = {1.0f, 0.96f, 0.90f};
	float groundToSun = 0.06f;     // the same ratio for light from below
	float exposureEV = 1.0f;
	int toneMapper = GLSL_TONEMAP_ACES_FITTED;

	LightingProperties ToLightingProperties() const
	{
		const float elevation = glm::radians(sunElevationDeg);
		const float azimuth = glm::radians(sunAzimuthDeg);
		const glm::vec3 towardsSun = {-std::cos(elevation) * std::sin(azimuth),
									  std::sin(elevation),
									  -std::cos(elevation) * std::cos(azimuth)};

		LightingProperties lighting{};
		lighting.m_lightDirections[0] = glm::vec4(towardsSun, 0.0f);
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

		const auto shaderDir = std::filesystem::path(SHADER_DIR).string();
		assert(!shaderDir.empty());

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			{
				std::ifstream file(std::string(RESOURCE_DIR) + "/Shaders/instanced-vertex.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}
				RenderSys::Shader vertexShader("Vertex", std::string(content.data(), content.size()));
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				vertexShader.SetIncludeDirectory(std::string(RESOURCE_DIR) + "/Shaders");
				m_renderer->SetShader(vertexShader);
			}

			{
				std::ifstream file(std::string(RESOURCE_DIR) + "/Shaders/pbr-fragment.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}

				RenderSys::Shader fragmentShader("Fragment", std::string(content.data(), content.size()));
				fragmentShader.type = RenderSys::ShaderType::SPIRV;
				fragmentShader.stage = RenderSys::ShaderStage::Fragment;
				fragmentShader.SetIncludeDirectory(std::string(RESOURCE_DIR) + "/Shaders");
				m_renderer->SetShader(fragmentShader);
			}
		}
		else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
		{
			std::ifstream file(shaderDir + "/combined.wgsl", std::ios::binary);
			std::vector<char> content((std::istreambuf_iterator<char>(file)),
										std::istreambuf_iterator<char>());

			if (!file.is_open()) {
				std::cerr << "Unable to open file." << std::endl;
				assert(false);
			}
			RenderSys::Shader shader("Combined", std::string(content.data(), content.size()));
			shader.type = RenderSys::ShaderType::WGSL;
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);
		}
		else
		{
			assert(false);
		}

		// glTF is right-handed: a left-handed camera shows it mirrored
		m_cameraController = std::make_unique<RenderSys::EditorCameraController>(30.0f, 0.01f, 500.0f, RenderSys::Handedness::Right);
		auto cameraEntity = m_scene->AddCamera(m_cameraController->GetCamera());
		m_cameraController->SetCameraEntity(cameraEntity, m_scene->m_Registry);

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

		std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(2);
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
		lightingUniformLayout.visibility = RenderSys::ShaderStage::Fragment; // only Fragment is needed
		lightingUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
		lightingUniformLayout.buffer.minBindingSize = sizeof(LightingProperties);

		m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
		m_renderer->CreateUniformBuffer(lightingUniformLayout.binding, sizeof(LightingProperties), 1);

		m_renderer->CreateBindGroup(bindingLayoutEntries);
		m_renderer->CreatePipeline(m_cameraController->GetCamera()->GetHandedness());
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
			m_cameraController->GetCamera()->SetAspectRatio((float)m_viewportWidth/ (float)m_viewportHeight);
        }

		if (m_renderer)
		{
			m_cameraController->OnUpdate(RenderSys::GetCameraInputFromWalnut());
			m_scene->Update();

			m_renderer->BeginFrame();
			m_renderer->BeginRenderPass();

			auto camera = m_cameraController->GetCamera();
			m_myUniformData.viewMatrix = camera->GetViewMatrix();
			m_myUniformData.projectionMatrix = camera->GetProjectionMatrix();
			m_myUniformData.cameraWorldPosition = camera->GetPosition();
			m_myUniformData.time = 0.0f;
			m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);
			m_lightingUniformData = m_lightRig.ToLightingProperties();
			m_renderer->SetUniformBufferData(1, &m_lightingUniformData, 0);
			m_renderer->BindResources();

			auto view = m_scene->m_Registry.view<RenderSys::MeshComponent, 
												RenderSys::TransformComponent, 
												RenderSys::InstanceTagComponent>();
			for (auto entity : view)
			{
				auto& meshComponent = view.get<RenderSys::MeshComponent>(entity);
				// Every primitive of the mesh draws all its instances, e.g. one per placed robot
				const uint32_t instanceCount = view.get<RenderSys::InstanceTagComponent>(entity).GetInstanceCount();
				for (auto& subMesh : meshComponent.m_Mesh->subMeshes)
					subMesh.m_InstanceCount = instanceCount;
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
		static ImVec4 newClearColorImgui = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGui::ColorEdit3("Clear Color", (float*)&newClearColorImgui); 
		glm::vec4 newClearColor = {newClearColorImgui.x, newClearColorImgui.y, newClearColorImgui.z, newClearColorImgui.w};
		if (newClearColor != m_clearColor)
		{
			m_clearColor = newClearColor;
			m_renderer->SetClearColor(m_clearColor);
		}
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
        ImGui::PopStyleVar();

		m_sceneHierarchyPanel->OnImGuiRender();
	}

private:
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

    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
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
	app->PushLayer<Renderer3DLayer>();
	return app;
}