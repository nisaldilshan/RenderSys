#include <array>

#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <imgui.h>

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
				layout (location = 1) in vec3 in_normal;
				layout (location = 2) in vec3 in_color;

				layout (location = 0) out vec3 out_color;
				layout (location = 1) out vec3 out_normal;

				void main() 
				{
					gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(aPos, 1.0);
					vec4 mult = ubo.modelMatrix * vec4(in_normal, 0.0);
					out_normal = mult.xyz;
					out_color = in_color;
				}
			)";
			RenderSys::Shader vertexShader("Vertex", vertexShaderSource);
			vertexShader.type = RenderSys::ShaderType::SPIRV;
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

				layout (location = 0) in vec3 in_color;
				layout (location = 1) in vec3 in_normal;

				layout (location = 0) out vec4 out_color;

				void main()
				{
					vec3 normal = normalize(in_normal);

					vec3 lightColor1 = vec3(1.0, 0.9, 0.6);
					vec3 lightColor2 = vec3(0.6, 0.9, 1.0);
					vec3 lightDirection1 = vec3(0.5, -0.9, 0.1);
					vec3 lightDirection2 = vec3(0.2, 0.4, 0.3);
					float shading1 = max(0.0, dot(lightDirection1, normal));
					float shading2 = max(0.0, dot(lightDirection2, normal));
					vec3 shading = shading1 * lightColor1 + shading2 * lightColor2;
					vec3 color = in_color * shading;

					out_color = vec4(color, ubo.color.a);
				}
			)";
			RenderSys::Shader fragmentShader("Fragment", fragmentShaderSource);
			fragmentShader.type = RenderSys::ShaderType::SPIRV;
			fragmentShader.stage = RenderSys::ShaderStage::Fragment;
			m_renderer->SetShader(fragmentShader);
		}
		else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
		{
			const char* shaderSource = R"(
			struct VertexInput {
				@location(0) position: vec3f,
				@location(1) normal: vec3f,
				@location(2) color: vec3f,
			};

			struct VertexOutput {
				@builtin(position) position: vec4f,
				@location(0) color: vec3f,
				@location(1) normal: vec3f,
			};

			/**
			 * A structure holding the value of our uniforms
			 */
			struct MyUniforms {
				projectionMatrix: mat4x4f,
				viewMatrix: mat4x4f,
				modelMatrix: mat4x4f,
				color: vec4f,
				time: f32,
			};

			// Instead of the simple uTime variable, our uniform variable is a struct
			@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
				// Forward the normal
				out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
				out.color = in.color;
				return out;
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				let normal = normalize(in.normal);

				let lightColor1 = vec3f(1.0, 0.9, 0.6);
				let lightColor2 = vec3f(0.6, 0.9, 1.0);
				let lightDirection1 = vec3f(0.5, -0.9, 0.1);
				let lightDirection2 = vec3f(0.2, 0.4, 0.3);
				let shading1 = max(0.0, dot(lightDirection1, normal));
				let shading2 = max(0.0, dot(lightDirection2, normal));
				let shading = shading1 * lightColor1 + shading2 * lightColor2;
				let color = in.color * shading;

				// Gamma-correction
				let corrected_color = pow(color, vec3f(2.2));
				return vec4f(corrected_color, uMyUniforms.color.a);
			}
			)";

			RenderSys::Shader shader("Combined", shaderSource);
			shader.type = RenderSys::ShaderType::WGSL;
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);
		}
		else
		{
			assert(false);
		}

		RenderSys::VertexBuffer vertexData;
		bool success = Geometry::loadGeometryFromObj(RESOURCE_DIR "/mammoth.obj", vertexData);
		if (!success) 
		{
			std::cerr << "Could not load geometry!" << std::endl;
			return;
		}

		std::vector<RenderSys::VertexAttribute> vertexAttribs(3);

		// Position attribute
		vertexAttribs[0].location = 0;
		vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[0].offset = 0;

		// Normal attribute
		vertexAttribs[1].location = 1;
		vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[1].offset = offsetof(RenderSys::Vertex, normal);

		// Color attribute
		vertexAttribs[2].location = 2;
		vertexAttribs[2].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[2].offset = offsetof(RenderSys::Vertex, color);

		RenderSys::VertexBufferLayout vertexBufferLayout;
		vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
		vertexBufferLayout.attributes = vertexAttribs.data();
		vertexBufferLayout.arrayStride = sizeof(RenderSys::Vertex);
		vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

		m_renderer->SetVertexBufferData(vertexData, vertexBufferLayout);
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

			// Create binding layout (don't forget to = Default)
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(1);
			RenderSys::BindGroupLayoutEntry& bGLayoutEntry = bindingLayoutEntries[0];
			bGLayoutEntry.setDefault();
			// The binding index as used in the @binding attribute in the shader
			bGLayoutEntry.binding = 0;
			// The stage that needs to access this resource
			bGLayoutEntry.visibility = RenderSys::ShaderStage::VertexAndFragment;
			bGLayoutEntry.buffer.type = RenderSys::BufferBindingType::Uniform;
			bGLayoutEntry.buffer.minBindingSize = sizeof(MyUniforms);
			// Make this binding dynamic so we can offset it between draw calls
			bGLayoutEntry.buffer.hasDynamicOffset = false;

			m_renderer->CreateUniformBuffer(bGLayoutEntry.binding, sizeof(MyUniforms), 2);
			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();
			
			const float time = static_cast<float>(glfwGetTime());


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

			m_renderer->BindResources();
			m_renderer->Render(0);
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
        const float imageWidth = m_renderer->GetWidth();
		const float imageHeight = m_renderer->GetHeight();
        ImGui::Image(m_renderer->GetDescriptorSet(), {imageWidth, imageHeight});
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