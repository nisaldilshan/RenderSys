#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Random.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer2D.h>
#include <imgui.h>

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer = std::make_shared<RenderSys::Renderer2D>();
	}

	virtual void OnDetach() override
	{
		if (m_renderer)
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
			m_renderer->Init();
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

			if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
			{
				const char* vertexShaderSource = R"(
					#version 450 core
					layout (location = 0) in vec2 aPos;
					layout (location = 1) in vec3 aColor; // Add color attribute
					layout (location = 0) out vec3 vColor; // Add color attribute

					void main() {
						gl_Position = vec4(aPos, 0.0, 1.0);
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

				@vertex
				fn vs_main(in: VertexInput) -> VertexOutput {
					var out: VertexOutput;
					out.position = vec4f(in.position, 0.0, 1.0);
					out.color = in.color; // forward to the fragment shader
					return out;
				}

				@fragment
				fn fs_main(in: VertexOutput) -> @location(0) vec4f {
					return vec4f(in.color, 1.0);
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


			// Vertex buffer
			// There are 2 floats per vertex, one for x and one for y.
			// But in the end this is just a bunch of floats to the eyes of the GPU,
			// the *layout* will tell how to interpret this.
			std::vector<float> vertexData = {
				// x ,  y ,  r ,  g ,  b
				-0.5, -0.5, 1.0, 0.0, 0.0,
				+0.5, -0.5, 0.0, 1.0, 0.0,
				+0.0, +0.5, 0.0, 0.0, 1.0,

				-0.55f, -0.5, 1.0, 1.0, 0.0,
				-0.05f, +0.5, 1.0, 0.0, 1.0,
				-0.55f, +0.5, 0.0, 1.0, 1.0
			};

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
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();
       		m_renderer->Render();
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
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer2D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer2DLayer>();
	return app;
}