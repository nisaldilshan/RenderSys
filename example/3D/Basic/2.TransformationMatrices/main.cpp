#include <array>

#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>

struct MyUniforms {
	// We add transform matrices
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

glm::mat4x4 makePerspectiveProj(float ratio, float near, float far, float focalLength)
{
	float divides = 1.0f / (far - near);
	float aaa[16] = {
		focalLength,         0.0,              0.0,               0.0,
			0.0,     focalLength * ratio,      0.0,               0.0,
			0.0,             0.0,         far * divides, -far * near * divides,
			0.0,             0.0,              1.0,               0.0,
	};
	return glm::transpose(glm::make_mat4(aaa));
}

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_shared<RenderSys::Renderer3D>();
		m_renderer->Init();	

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			const char* vertexShaderSource = R"(
				#version 450 core
				layout(binding = 0) uniform UniformBufferObject {
					mat4 projectionMatrix;
					mat4 viewMatrix;
					mat4 modelMatrix;
					vec4 color;
					float time;
					float _pad[3];
				} ubo;
				layout (location = 0) in vec3 aPos;
				layout (location = 1) in vec3 aColor; // Add color attribute
				layout (location = 0) out vec3 vColor; // Add color attribute

				void main() 
				{
					gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(aPos, 1.0);
					vColor = aColor; // Pass color to fragment shader
				}
			)";
			RenderSys::Shader vertexShader("Vertex");
			vertexShader.type = RenderSys::ShaderType::SPIRV;
			vertexShader.shaderSrc = vertexShaderSource;
			vertexShader.stage = RenderSys::ShaderStage::Vertex;
			m_renderer->SetShader(vertexShader);

			const char* fragmentShaderSource = R"(
				#version 450
				layout(binding = 0) uniform UniformBufferObject {
					mat4 projectionMatrix;
					mat4 viewMatrix;
					mat4 modelMatrix;
					vec4 color;
					float time;
					float _pad[3];
				} ubo;
				layout(location = 0) in vec3 vColor;
				layout(location = 0) out vec4 FragColor;

				void main()
				{
					FragColor = vec4(vColor, 1.0) * ubo.color;
				}
			)";
			RenderSys::Shader fragmentShader("Fragment");
			fragmentShader.type = RenderSys::ShaderType::SPIRV;
			fragmentShader.shaderSrc = fragmentShaderSource;
			fragmentShader.stage = RenderSys::ShaderStage::Fragment;
			m_renderer->SetShader(fragmentShader);
		}
		else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
		{
			const char* shaderSource = R"(
			struct VertexInput {
				@location(0) position: vec3f,
				@location(1) color: vec3f,
			};

			struct VertexOutput {
				@builtin(position) position: vec4f,
				@location(0) color: vec3f,
			};

			struct MyUniforms {
				projectionMatrix: mat4x4f,
				viewMatrix: mat4x4f,
				modelMatrix: mat4x4f,
				color: vec4f,
				time: f32,
			};

			@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

			fn vs_main_optionB(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
				out.color = in.color;
				return out;
			}

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				return vs_main_optionB(in);
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				let color = in.color * uMyUniforms.color.rgb;
				// Gamma-correction
				let corrected_color = pow(color, vec3f(2.2));
				return vec4f(corrected_color, uMyUniforms.color.a);
			}
			)";
			
			RenderSys::Shader shader("Combined");
			shader.type = RenderSys::ShaderType::WGSL;
			shader.shaderSrc = shaderSource;
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);
		}
		else
		{
			assert(false);
		}
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

			RenderSys::VertexBuffer vertexData;
			std::vector<uint32_t> indexData;
			auto success = Geometry::load3DGeometry(RESOURCE_DIR "/model.txt", vertexData, indexData, 3, true);
			if (!success) 
			{
				std::cerr << "Could not load geometry!" << std::endl;
				return;
			}

			// Vertex fetch
			// We now have 2 attributes
			std::vector<RenderSys::VertexAttribute> vertexAttribs(2);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// Color attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[1].offset = offsetof(RenderSys::Vertex, color);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			vertexBufferLayout.arrayStride = sizeof(RenderSys::Vertex);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

			m_renderer->SetVertexBufferData(vertexData, vertexBufferLayout);
			m_renderer->SetIndexBufferData(indexData);

			// Create binding layout (don't forget to = Default)
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(1);
			RenderSys::BindGroupLayoutEntry& bGLayoutEntry = bindingLayoutEntries[0];
			bGLayoutEntry.setDefault();
			// The binding index as used in the @binding attribute in the shader
			bGLayoutEntry.binding = 0;
			// The stage that needs to access this resource
			bGLayoutEntry.visibility = static_cast<RenderSys::ShaderStage>(
				static_cast<uint32_t>(RenderSys::ShaderStage::Vertex) | static_cast<uint32_t>(RenderSys::ShaderStage::Fragment));
			bGLayoutEntry.buffer.type = RenderSys::BufferBindingType::Uniform;
			bGLayoutEntry.buffer.minBindingSize = sizeof(MyUniforms);
			// Make this binding dynamic so we can offset it between draw calls
			bGLayoutEntry.buffer.hasDynamicOffset = true;

			m_renderer->CreateUniformBuffer(bGLayoutEntry.binding, sizeof(MyUniforms), 2);
			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			float time = static_cast<float>(glfwGetTime());

			// Upload first value
			constexpr float PI = 3.14159265358979323846f;
			float angle2 = 3.0f * PI / 4.0f;
			glm::vec3 focalPoint(0.0, 0.0, -2.0);			

			glm::mat4x4 V(1.0);
			V = glm::translate(V, -focalPoint);
			V = glm::rotate(V, -angle2, glm::vec3(1.0, 0.0, 0.0));
			m_uniformData.viewMatrix = V;
			
			float focalLength = 2.0;
			float fov = 2 * glm::atan(1 / focalLength);
			float ratio = m_viewportWidth / m_viewportHeight;
			float near = 0.01f;
			float far = 100.0f;
			//m_uniformData.projectionMatrix = glm::perspective(fov, ratio, near, far);
			m_uniformData.projectionMatrix = makePerspectiveProj(m_viewportWidth / m_viewportHeight, 0.01, 100.0, 2.0);

			glm::mat4x4 M1(1.0);
			float angle1 = time * 0.9f;
			M1 = glm::rotate(M1, angle1, glm::vec3(0.0, 0.0, 1.0));
			M1 = glm::translate(M1, glm::vec3(0.5, 0.0, 0.0));
			M1 = glm::scale(M1, glm::vec3(0.3f));
			m_uniformData.modelMatrix = M1;

			m_uniformData.time = time;
			m_uniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
			m_renderer->SetUniformBufferData(0, &m_uniformData, 0);

			// Upload second value
			glm::mat4x4 M2(1.0);
			angle1 = time * 1.1f;
			M2 = glm::rotate(M2, angle1, glm::vec3(0.0, 0.0, 1.0));
			M2 = glm::translate(M2, glm::vec3(0.5, 0.0, 0.0));
			M2 = glm::scale(M2, glm::vec3(0.3f));
			m_uniformData.modelMatrix = M2;

			m_uniformData.time = time;
			m_uniformData.color = { 1.0f, 1.0f, 1.0f, 0.7f };
			m_renderer->SetUniformBufferData(0, &m_uniformData, 1);
			m_renderer->BindResources();

			m_renderer->RenderIndexed(0);
			m_renderer->RenderIndexed(1);
			m_renderer->EndRenderPass();
		}
       		

        m_lastRenderTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", m_lastRenderTime);
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
    std::shared_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;

	MyUniforms m_uniformData;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer3DLayer>();
	return app;
}