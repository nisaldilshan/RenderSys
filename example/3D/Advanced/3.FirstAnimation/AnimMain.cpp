#include <array>
#include <fstream>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <RenderSys/Camera.h>
#include <RenderSys/Scene/Scene.h>
#include <imgui.h>

struct MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

struct LightingUniforms {
    std::array<glm::vec4, 2> directions;
    std::array<glm::vec4, 2> colors;
};
static_assert(sizeof(LightingUniforms) % 16 == 0);

struct alignas(16) MaterialItem {
	std::array<float, 4> color;
	float hardness = 16.0f;
	float kd = 2.0f;
	float ks = 0.3f;
	float workflow = 1.0f;
	float metallic = 0.0f;
	float roughness = 0.0f;
	int colorTextureSet;
    int PhysicalDescriptorTextureSet;
    int normalTextureSet;
};

struct MaterialUniforms {
	std::array<MaterialItem, 32> materials;
};
static_assert(sizeof(MaterialUniforms) % 16 == 0);

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		if (!loadScene())
		{
			assert(false);
			return;
		}

		m_renderer = std::make_unique<RenderSys::Renderer3D>();
		m_renderer->Init();

		const auto shaderDir = std::filesystem::path(SHADER_DIR).string();
		assert(!shaderDir.empty());

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			{
				std::ifstream file(shaderDir + "/anim-vertex.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}
				RenderSys::Shader vertexShader("Vertex");
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.shaderSrc = std::string(content.data(), content.size());
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				m_renderer->SetShader(vertexShader);
			}

			{
				std::ifstream file(shaderDir + "/anim-fragment.glsl", std::ios::binary);
				std::vector<char> content((std::istreambuf_iterator<char>(file)),
											std::istreambuf_iterator<char>());

				if (!file.is_open()) {
					std::cerr << "Unable to open file." << std::endl;
					assert(false);
				}

				RenderSys::Shader fragmentShader("Fragment");
				fragmentShader.type = RenderSys::ShaderType::SPIRV;
				fragmentShader.shaderSrc = std::string(content.data(), content.size());
				fragmentShader.stage = RenderSys::ShaderStage::Fragment;
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
			RenderSys::Shader shader("Combined");
			shader.type = RenderSys::ShaderType::WGSL;
			shader.shaderSrc = std::string(content.data(), content.size());
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);
		}
		else
		{
			assert(false);
		}

		const auto& sceneTextures = m_scene->getTextures();
		std::vector<RenderSys::TextureDescriptor> texDescriptors;
		for (const auto &sceneTexture : sceneTextures)
		{
			texDescriptors.emplace_back(sceneTexture.GetDescriptor());
		}
		m_renderer->CreateTextures(texDescriptors);
		m_renderer->CreateTextureSamplers(m_scene->getSamplers());
		m_camera = std::make_unique<Camera::PerspectiveCamera>(30.0f, 0.01f, 100.0f);
	}

	virtual void OnDetach() override
	{
		m_renderer->Destroy();
	}

	virtual void OnUpdate(float ts) override
	{
        Walnut::Timer timer;
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

        if (!m_renderer ||
            m_viewportWidth != m_renderer->GetWidth() ||
            m_viewportHeight != m_renderer->GetHeight())
        {
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

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

			assert(m_vertexBuffer.size() > 0);
			m_renderer->SetVertexBufferData(m_vertexBuffer, vertexBufferLayout);
			assert(m_indexData.size() > 0);
			m_renderer->SetIndexBufferData(m_indexData);

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
			lightingUniformLayout.visibility = RenderSys::ShaderStage::Fragment; // only Fragment is needed
			lightingUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			lightingUniformLayout.buffer.minBindingSize = sizeof(LightingUniforms);

			// Material Uniforms
			RenderSys::BindGroupLayoutEntry& materialUniformLayout = bindingLayoutEntries[2];
			materialUniformLayout.setDefault();
			materialUniformLayout.binding = 2;
			materialUniformLayout.visibility = RenderSys::ShaderStage::Fragment; // only Fragment is needed
			materialUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			materialUniformLayout.buffer.minBindingSize = sizeof(MaterialUniforms);

			m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
			m_renderer->CreateUniformBuffer(lightingUniformLayout.binding, sizeof(LightingUniforms), 1);
			m_renderer->CreateUniformBuffer(materialUniformLayout.binding, sizeof(MaterialUniforms), 1);

			m_renderer->CreateBindGroup(bindingLayoutEntries);
			const auto& materials =  m_scene->getMaterials();
			m_renderer->CreateMaterialBindGroups(materials);
			m_renderer->CreatePipeline();
			m_camera->SetViewportSize((float)m_viewportWidth, (float)m_viewportHeight);

			int matCount = 0;
			assert(materials.size() <= 32);
			for (const auto& material : materials)
			{
				const auto& baseColor = material.baseColorFactor;
				m_materialUniformData.materials[matCount].color = {
					baseColor.x, baseColor.y, baseColor.z, baseColor.w
				};
				m_materialUniformData.materials[matCount].metallic = material.metallicFactor;
				m_materialUniformData.materials[matCount].roughness = material.roughnessFactor;
				m_materialUniformData.materials[matCount].colorTextureSet 
					= material.baseColorTextureIndex == -1 ? -1 : 0; // material.texCoordSets.baseColor
				m_materialUniformData.materials[matCount].PhysicalDescriptorTextureSet 
					= material.normalTextureIndex == -1 ? -1 : 0; // material.texCoordSets.normal
				m_materialUniformData.materials[matCount].normalTextureSet 
					= material.metallicRoughnessTextureIndex == -1 ? -1 : 0;
				matCount++;
			}

			m_renderer->SetClearColor({ 0.45f, 0.55f, 0.60f, 1.00f });
        }

		if (m_renderer)
		{
			m_camera->OnUpdate();

			m_renderer->BeginRenderPass();

			glm::mat4x4 M1(1.0);
			M1 = glm::rotate(M1, 0.0f, glm::vec3(0.0, 0.0, 1.0));
			M1 = glm::translate(M1, glm::vec3(0.0, 0.0, 0.0));
			M1 = glm::scale(M1, glm::vec3(0.005f));
			// set uniform data
			m_myUniformData.viewMatrix = m_camera->GetViewMatrix();
			m_myUniformData.projectionMatrix = m_camera->GetProjectionMatrix();
			m_myUniformData.modelMatrix = M1;
			m_myUniformData.cameraWorldPosition = m_camera->GetPosition();
			m_myUniformData.time = 0.0f;
			m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);
			m_lightingUniformData.directions[0] = { 0.5f, 0.5f, 0.5f, 0.0f };
			m_lightingUniformData.directions[1] = { -0.5f, -0.5f, -0.5f, 0.0f };
			m_lightingUniformData.colors[0] = { 1.0f, 0.9f, 0.6f, 1.0f };
			m_lightingUniformData.colors[1] = { 0.6f, 0.9f, 1.0f, 1.0f };
			m_renderer->SetUniformBufferData(1, &m_lightingUniformData, 0);
			m_renderer->SetUniformBufferData(2, &m_materialUniformData, 0);

			m_renderer->BindResources();
			for (const auto &rootNode : m_scene->getRootNodes())
			{
				const RenderSys::Mesh mesh = rootNode->getMesh();
				m_renderer->RenderMesh(mesh, 0);
			}
			
			m_renderer->EndRenderPass();
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
		if (m_renderer)
        	ImGui::Image(m_renderer->GetDescriptorSet(), {(float)m_renderer->GetWidth(),(float)m_renderer->GetWidth()});
		ImGui::End();
        ImGui::PopStyleVar();

		ImGui::ShowDemoWindow();
	}

private:
	bool loadScene()
	{
		m_scene = std::make_unique<RenderSys::Scene>();
		if (!m_scene->load(RESOURCE_DIR "/Models/Sponza/glTF/Sponza.gltf", ""))
		{
			std::cout << "Error loading GLTF model!" << std::endl;
			return false;
		}

		m_scene->populate();
		m_vertexBuffer.resize(m_scene->getVertexBuffer().size());
		for (size_t i = 0; i < m_vertexBuffer.size(); i++)
		{
			m_vertexBuffer[i].position = m_scene->getVertexBuffer()[i].pos;
			m_vertexBuffer[i].normal = m_scene->getVertexBuffer()[i].normal;
			m_vertexBuffer[i].texcoord0 = m_scene->getVertexBuffer()[i].uv0;
			m_vertexBuffer[i].tangent = m_scene->getVertexBuffer()[i].tangent;
		}
		
		m_indexData.resize(m_scene->getIndexBuffer().size());
		for (size_t i = 0; i < m_indexData.size(); i++)
		{
			m_indexData[i] = m_scene->getIndexBuffer()[i];
		}

		m_scene->printNodeGraph();
		return true;
	}

    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	glm::vec4 m_clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyUniforms m_myUniformData;
	LightingUniforms m_lightingUniformData;
	MaterialUniforms m_materialUniformData;
	RenderSys::VertexBuffer m_vertexBuffer;
	std::vector<uint32_t> m_indexData;
	std::unique_ptr<Camera::PerspectiveCamera> m_camera;
	std::unique_ptr<RenderSys::Scene> m_scene;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer3DLayer>();
	return app;
}