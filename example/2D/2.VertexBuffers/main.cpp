#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer2D.h>

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
				RenderSys::Shader vertexShader("Vertex");
				const char* vertexShaderSource = R"(
					#version 450 core
					layout (location = 0) in vec2 aPos;

					void main() {
						gl_Position = vec4(aPos, 0.0, 1.0);
					}
				)";
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.shaderSrc = vertexShaderSource;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				m_renderer->SetShader(vertexShader);

				RenderSys::Shader fragmentShader("Fragment");
				const char* fragmentShaderSource = R"(
					#version 450

					layout(location = 0) out vec4 FragColor;

					void main()
					{
						FragColor = vec4(1.0, 0.4000000059604644775390625, 0.0, 1.0);
					}
				)";
				fragmentShader.type = RenderSys::ShaderType::SPIRV;
				fragmentShader.shaderSrc = fragmentShaderSource;
				fragmentShader.stage = RenderSys::ShaderStage::Fragment;
				m_renderer->SetShader(fragmentShader);
			}
			else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
			{
				RenderSys::Shader shader("Combined");
				const char* shaderSource = R"(
				// The `@location(0)` attribute means that this input variable is described
				// by the vertex buffer layout at index 0 in the `pipelineDesc.vertex.buffers`
				// array.
				// The type `vec2f` must comply with what we will declare in the layout.
				// The argument name `in_vertex_position` is up to you, it is only internal to
				// the shader code!
					@vertex
					fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
						return vec4f(in_vertex_position, 0.0, 1.0);
					}

					@fragment
					fn fs_main() -> @location(0) vec4f {
						return vec4f(1.0, 0.4, 0.0, 1.0);
					}
				)";

				shader.type = RenderSys::ShaderType::WGSL;
				shader.shaderSrc = shaderSource;
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
			const std::vector<float> vertexData = {
				-0.5, -0.5,
				+0.5, -0.5,
				+0.0, +0.5,

				-0.55f, -0.5,
				-0.05f, +0.5,
				-0.55f, +0.5,

				+0.275f, +0.05,
				+0.5f, +0.5,
				+0.05f, +0.5
			};

			RenderSys::VertexAttribute vertexAttrib;
			// == Per attribute ==
			// Corresponds to @location(...)
			vertexAttrib.location = 0;
			// Means vec2f in the shader
			vertexAttrib.format = RenderSys::VertexFormat::Float32x2;
			// Index of the first element
			vertexAttrib.offset = 0;

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = 1;
			vertexBufferLayout.attributes = &vertexAttrib;
			vertexBufferLayout.arrayStride = 2 * sizeof(float);
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