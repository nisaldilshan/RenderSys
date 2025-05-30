#include <array>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer2D.h>
#include <RenderSys/Geometry.h>
#include <imgui.h>


//The same structure as in the shader, replicated in C++
struct MyUniforms {
	// offset = 0 * sizeof(vec4f) -> OK
	std::array<float, 4> color;
	// offset = 16 = 4 * sizeof(f32) -> OK
	float time;
	// Add padding to make sure the struct is host-shareable
	float _pad[3];
};
static_assert(sizeof(MyUniforms) % 16 == 0);

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_shared<RenderSys::Renderer2D>();
	}

	virtual void OnDetach() override
	{

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
			m_renderer->Init();
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

			if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
			{
				const char* vertexShaderSource = R"(
					#version 450 core
					layout(binding = 0) uniform UniformBufferObject {
						vec4 color;
						float time;
						float _pad[3];
					} ubo;
					layout (location = 0) in vec2 aPos;
					layout (location = 1) in vec3 aColor; // Add color attribute
					layout (location = 0) out vec3 vColor; // Add color attribute

					void main() {
						vec2 offset = vec2(-0.6875, -0.463);
						offset += 0.3 * vec2(cos(ubo.time), sin(ubo.time));
						gl_Position = vec4(aPos+offset, 0.0, 1.0);
						vColor = aColor; // Pass color to fragment shader
					}
				)";
				RenderSys::Shader vertexShader("Vertex", vertexShaderSource);
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				m_renderer->SetShader(vertexShader);

				const char* fragmentShaderSource = R"(
					#version 450
					layout(location = 0) in vec3 vColor;
					layout(location = 0) out vec4 FragColor;

					void main()
					{
						FragColor = vec4(vColor, 1.0); // Use the color from the vertex shader
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
				/**
				 * A structure with fields labeled with vertex attribute locations can be used
				 * as input to the entry point of a shader.
				 */
				struct VertexInput {
					@location(0) position: vec2f,
					@location(1) color: vec3f,
				};

				/**
				 * A structure with fields labeled with builtins and locations can also be used
				 * as *output* of the vertex shader, which is also the input of the fragment
				 * shader.
				 */
				struct VertexOutput {
					@builtin(position) position: vec4f,
					// The location here does not refer to a vertex attribute, it just means
					// that this field must be handled by the rasterizer.
					// (It can also refer to another field of another struct that would be used
					// as input to the fragment shader.)
					@location(0) color: vec3f,
				};

				/**
				 * A structure holding the value of our uniforms
				 */
				struct MyUniforms {
					color: vec4f,
					time: f32,
				};

				// Instead of the simple uTime variable, our uniform variable is a struct
				@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

				@vertex
				fn vs_main(in: VertexInput) -> VertexOutput {
					var out: VertexOutput;
					let ratio = 640.0 / 480.0;
					var offset = vec2f(-0.6875, -0.463);
					offset += 0.3 * vec2f(cos(uMyUniforms.time), sin(uMyUniforms.time));
					out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
					out.color = in.color; // forward to the fragment shader
					return out;
				}

				@fragment
				fn fs_main(in: VertexOutput) -> @location(0) vec4f {
					// We apply a gamma-correction to the color
					let corrected_color = pow(in.color, vec3f(2.2));
					return vec4f(corrected_color, 1.0);
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


			//
			std::vector<float> vertexData;
			std::vector<uint16_t> indexData;
			auto success = Geometry::load2DGeometry(RESOURCE_DIR "/model.txt", vertexData, indexData);
			if (!success) 
			{
				std::cerr << "Could not load geometry!" << std::endl;
				return;
			}
			//

			// Vertex fetch
			// We now have 2 attributes
			std::vector<RenderSys::VertexAttribute> vertexAttribs(2);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[0].offset = 0;

			// Color attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3; // different type!
			vertexAttribs[1].offset = 2 * sizeof(float); // non null offset!

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			// stride
			vertexBufferLayout.arrayStride = 5 * sizeof(float);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;


			m_renderer->SetVertexBufferData(vertexData.data(), vertexData.size() * 4, vertexBufferLayout);
			m_renderer->SetIndexBufferData(indexData);

			// Create binding layout (don't forget to = Default)
			RenderSys::BindGroupLayoutEntry bGLayoutEntry;// = wgpu::Default;
			// The binding index as used in the @binding attribute in the shader
			bGLayoutEntry.binding = 0;
			// The stage that needs to access this resource
			bGLayoutEntry.visibility = RenderSys::ShaderStage::Vertex;
			bGLayoutEntry.buffer.type = RenderSys::BufferBindingType::Uniform;
			bGLayoutEntry.buffer.minBindingSize = sizeof(MyUniforms);
			bGLayoutEntry.buffer.hasDynamicOffset = false;

			m_renderer->CreateUniformBuffer(1, sizeof(MyUniforms));
			m_renderer->CreateBindGroup(bGLayoutEntry);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();
			// Update uniform buffer
			m_uniformData.time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
			m_renderer->SetUniformBufferData(&m_uniformData, 0);

			m_renderer->RenderIndexed(0, 0);
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
    std::shared_ptr<RenderSys::Renderer2D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;

	MyUniforms m_uniformData;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer2D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer2DLayer>();
	return app;
}