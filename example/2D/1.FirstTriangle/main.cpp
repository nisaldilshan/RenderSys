#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
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
					#version 450

					vec2 positions[3] = vec2[](
						vec2(0.0, -0.5),
						vec2(0.5, 0.5),
						vec2(-0.5, 0.5)
					);

					void main() {
						gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
					}
				)";
				RenderSys::Shader vertexShader("Vertex", vertexShaderSource);
				vertexShader.type = RenderSys::ShaderType::SPIRV;
				vertexShader.stage = RenderSys::ShaderStage::Vertex;
				m_renderer->SetStandaloneShader(vertexShader, 3);

				const char* fragmentShaderSource = R"(
					#version 450

					layout(location = 0) out vec4 FragColor;
					layout(location = 0) in vec2 texCoord;

					void main()
					{
						FragColor = vec4(1.0, 0.4000000059604644775390625, 0.0, 1.0);
					}
				)";
				RenderSys::Shader fragmentShader("Fragment", fragmentShaderSource);
				fragmentShader.type = RenderSys::ShaderType::SPIRV;
				fragmentShader.stage = RenderSys::ShaderStage::Fragment;
				m_renderer->SetStandaloneShader(fragmentShader, 3);
			}
			else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
			{
				const char* shaderSource = R"(
					@vertex
					fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
						var p = vec2f(0.0, 0.0);
						if (in_vertex_index == 0u) {
							p = vec2f(-0.5, -0.5);
						} else if (in_vertex_index == 1u) {
							p = vec2f(0.5, -0.5);
						} else {
							p = vec2f(0.0, 0.5);
						}
						return vec4f(p, 0.0, 1.0);
					}

					@fragment
					fn fs_main() -> @location(0) vec4f {
						return vec4f(0.0, 0.4, 1.0, 1.0);
					}
				)";
				RenderSys::Shader shader("Combined", shaderSource);
				shader.type = RenderSys::ShaderType::WGSL;
				shader.stage = RenderSys::ShaderStage::VertexAndFragment;
				m_renderer->SetStandaloneShader(shader, 3);
			}
			else
			{
				assert(false);
			}
			
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();
       		m_renderer->SimpleRender();
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