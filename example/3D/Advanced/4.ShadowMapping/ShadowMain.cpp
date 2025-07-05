#include <array>
#include <fstream>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Camera/PerspectiveCamera.h>
#include <RenderSys/Camera/EditorCameraController.h>
#include <RenderSys/Scene/Model.h>
#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Components/TagAndIDComponents.h>
#include <RenderSys/Components/LightComponents.h>
#include <RenderSys/Scene/Scene.h>
#include <RenderSys/Scene/SceneHierarchyPanel.h>
#include <imgui.h>

struct alignas(16) MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

struct LightingUniforms {
    std::array<glm::vec4, 1> lightDirections;
    std::array<glm::vec4, 1> lightColors;
	std::array<glm::mat4x4, 1> lightViewProjections;
};
static_assert(sizeof(LightingUniforms) % 16 == 0);

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_unique<RenderSys::Renderer3D>();
		m_renderer->Init();

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
				std::ifstream file(shaderDir + "/ShadowMain-vert.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}
				RenderSys::Shader vertexShader("Vertex", std::string(content.data(), content.size()));
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				vertexShader.SetIncludeDirectory(shaderDir + "/../../../Resources/Shaders");
				m_renderer->SetShader(vertexShader);
			}

			{
				std::ifstream file(shaderDir + "/ShadowMain-frag.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}

				RenderSys::Shader fragmentShader("Fragment", std::string(content.data(), content.size()));
				fragmentShader.type = RenderSys::ShaderType::SPIRV;
				fragmentShader.stage = RenderSys::ShaderStage::Fragment;
				fragmentShader.SetIncludeDirectory(shaderDir + "/../../../Resources/Shaders");
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

		m_cameraController = std::make_unique<RenderSys::EditorCameraController>(30.0f, 0.01f, 500.0f);

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
		vertexAttribs[4].format = RenderSys::VertexFormat::Float32x3;
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
		createLights();

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
		lightingUniformLayout.buffer.minBindingSize = sizeof(LightingUniforms);

		m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
		m_renderer->CreateUniformBuffer(lightingUniformLayout.binding, sizeof(LightingUniforms), 1);

		m_renderer->CreateBindGroup(bindingLayoutEntries);
		m_renderer->CreatePipeline();
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
			m_cameraController->OnUpdate();
			m_scene->Update();

			auto camera = m_cameraController->GetCamera();
			m_myUniformData.viewMatrix = camera->GetViewMatrix();
			m_myUniformData.projectionMatrix = camera->GetProjectionMatrix();
			m_myUniformData.cameraWorldPosition = camera->GetPosition();
			m_myUniformData.time = 0.0f;
			m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);

			auto lightView = m_scene->m_Registry.view<RenderSys::DirectionalLightComponent, 
												RenderSys::TransformComponent>();
			for (auto entity : lightView)
			{
				auto& lightComponent = lightView.get<RenderSys::DirectionalLightComponent>(entity);
				m_lightingUniformData.lightDirections[0] = { lightComponent.m_Direction, 0.0f };
				m_lightingUniformData.lightColors[0] = { lightComponent.m_Color, 1.0f };
			}
			m_renderer->SetUniformBufferData(1, &m_lightingUniformData, 0);

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
				auto& instanceTagComponent = meshView.get<RenderSys::InstanceTagComponent>(entity);
				instanceTagComponent.GetInstanceBuffer()->Update();
				auto mesh = meshComponent.m_Mesh;
				m_renderer->RenderMesh(*mesh);
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
		ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
        m_viewportHeight = ImGui::GetContentRegionAvail().y;
		const float imageWidth = m_renderer->GetWidth();
		const float imageHeight = m_renderer->GetHeight();
        ImGui::Image(m_renderer->GetDescriptorSet(), {imageWidth, imageHeight});
		ImGui::End();

		m_renderer->OnImGuiRender();
        ImGui::PopStyleVar();

		m_sceneHierarchyPanel->OnImGuiRender();
	}

private:
	void createLights()
	{
		m_scene->AddDirectionalLight(glm::vec3( 0.5f, 0.5f, 0.5f), glm::vec3( 1.0f, 1.0f, 1.0f));
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

    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	glm::vec4 m_clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyUniforms m_myUniformData;
	LightingUniforms m_lightingUniformData;
	std::unique_ptr<RenderSys::EditorCameraController> m_cameraController;
	std::shared_ptr<RenderSys::Scene> m_scene;
	std::vector<RenderSys::Model> m_models;
	std::unique_ptr<RenderSys::SceneHierarchyPanel> m_sceneHierarchyPanel;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer3DLayer>();
	return app;
}