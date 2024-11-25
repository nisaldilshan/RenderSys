#include <array>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>

struct VertexAttributes {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;
};

struct MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_shared<RenderSys::Renderer3D>();
		m_renderer->Init();	
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
			if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
			{
				const char* vertexShaderSource = R"(
					#version 460
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
					layout (location = 3) in vec2 in_uv;

					layout (location = 0) out vec3 out_color;
					layout (location = 1) out vec2 out_uv;

					void main() 
					{
						gl_Position = ubo.projectionMatrix * ubo.viewMatrix * ubo.modelMatrix * vec4(aPos, 1.0);
						vec4 mult = ubo.modelMatrix * vec4(in_normal, 0.0);
						out_color = in_color;
						out_uv = in_uv;
					}
				)";
				RenderSys::Shader vertexShader("Vertex");
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.shaderSrc = vertexShaderSource;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				m_renderer->SetShader(vertexShader);

				const char* fragmentShaderSource = R"(
					#version 460

					layout(binding = 0) uniform UniformBufferObject {
						mat4 projectionMatrix;
						mat4 viewMatrix;
						mat4 modelMatrix;
						vec4 color;
						float time;
						float _pad[3];
					} ubo;

					layout(binding = 1) uniform texture2D tex;
					layout(binding = 2) uniform sampler s;

					layout (location = 0) in vec3 in_color;
					layout (location = 1) in vec2 in_uv;

					layout (location = 0) out vec4 out_color;

					void main()
					{
						vec3 texColor = texture(sampler2D(tex, s), in_uv).rgb; 

						out_color = vec4(texColor, ubo.color.a);
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
					@location(1) normal: vec3f,
					@location(2) color: vec3f,
					@location(3) uv: vec2f,
				};

				struct VertexOutput {
					@builtin(position) position: vec4f,
					@location(0) color: vec3f,
					@location(1) normal: vec3f,
					@location(2) uv: vec2f,
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

				@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
				@group(0) @binding(1) var gradientTexture: texture_2d<f32>;
				@group(0) @binding(2) var textureSampler: sampler;

				@vertex
				fn vs_main(in: VertexInput) -> VertexOutput {
					var out: VertexOutput;
					out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
					// Forward the normal
					out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
					out.color = in.color;
					out.uv = in.uv * 2.0;

					return out;
				}

				@fragment
				fn fs_main(in: VertexOutput) -> @location(0) vec4f {
					// We remap UV coords to actual texel coordinates
					let color = textureSample(gradientTexture, textureSampler, in.uv).rgb;

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

			std::vector<VertexAttributes> vertexData;
			bool success = Geometry::loadGeometryFromObjWithUV<VertexAttributes>(RESOURCE_DIR "/Meshes/cube.obj", vertexData);
			if (!success) 
			{
				std::cerr << "Could not load geometry!" << std::endl;
				assert(false);
				return;
			}

			std::vector<RenderSys::VertexAttribute> vertexAttribs(4);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// Normal attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

			// Color attribute
			vertexAttribs[2].location = 2;
			vertexAttribs[2].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[2].offset = offsetof(VertexAttributes, color);

			// UV attribute
			vertexAttribs[3].location = 3;
			vertexAttribs[3].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

			m_renderer->SetVertexBufferData(vertexData.data(), vertexData.size() * sizeof(VertexAttributes), vertexBufferLayout);

			// Since we now have 2 bindings, we use a vector to store them
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(3);
			// The uniform buffer binding that we already had
			RenderSys::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
			uniformBindingLayout.setDefault();
			uniformBindingLayout.binding = 0;
			uniformBindingLayout.visibility = RenderSys::ShaderStage::VertexAndFragment;
			uniformBindingLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			uniformBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
			uniformBindingLayout.buffer.hasDynamicOffset = true;

			// The texture binding
			RenderSys::BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
			textureBindingLayout.setDefault();
			textureBindingLayout.binding = 1;
			textureBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			textureBindingLayout.texture.sampleType = RenderSys::TextureSampleType::Float;
			textureBindingLayout.texture.viewDimension = RenderSys::TextureViewDimension::_2D;

			// The sampler binding
			RenderSys::BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
			samplerBindingLayout.setDefault();
			samplerBindingLayout.binding = 2;
			samplerBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			samplerBindingLayout.sampler.type = RenderSys::SamplerBindingType::Filtering;

			m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 2);

			constexpr uint32_t texWidth = 256;
			constexpr uint32_t texHeight = 256;
			std::vector<uint8_t> pixels(4 * texWidth * texHeight);

			for (uint32_t i = 0; i < texWidth; ++i) {
				for (uint32_t j = 0; j < texHeight; ++j) {
					uint8_t *p = &pixels[4 * (j * texWidth + i)];
					p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
					p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
					p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
					p[3] = 255; // a
				}
			}
			m_renderer->CreateTextureSampler();
			m_renderer->CreateTexture(textureBindingLayout.binding, texWidth, texHeight, pixels.data(), 8);

			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			const float time = static_cast<float>(glfwGetTime());
			constexpr float PI = 3.14159265358979323846f;		
			m_uniformData.viewMatrix = glm::lookAt(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0, 0, 1));
			m_uniformData.projectionMatrix = glm::perspective(30 * (PI / 180), (float)(m_viewportWidth / m_viewportHeight), 0.01f, 100.0f);

			glm::mat4x4 M1(1.0);
			float angle1 = time * 0.9f;
			M1 = glm::rotate(M1, angle1, glm::vec3(0.0, 0.0, 1.0));
			M1 = glm::translate(M1, glm::vec3(1.0, 0.0, 0.0));
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

			m_renderer->Render(0);
			m_renderer->Render(1);
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