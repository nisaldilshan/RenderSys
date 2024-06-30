#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>

#include "../../../src/Compute.h"
#include <GLFW/glfw3.h>

constexpr uint32_t g_bufferSize = 64 * sizeof(float);

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_compute.reset();
		m_compute = std::make_unique<Compute>();

		const char* shaderSource = R"(
		@group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
		@group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;

		// The function to evaluate for each element of the processed buffer
		fn f(x: f32) -> f32 {
			return 2.0 * x + 1.0;
		}

		@compute @workgroup_size(32)
		fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
			// Apply the function f to the buffer element at index id.x:
			outputBuffer[id.x] = f(inputBuffer[id.x]);
		}
		)";
		m_compute->SetShader(shaderSource);

		
		m_compute->CreateBuffer(g_bufferSize, ComputeBuf::BufferType::Input, "INPUT_BUFFER");
		m_compute->CreateBuffer(g_bufferSize, ComputeBuf::BufferType::Output, "OUTPUT_BUFFER");
		m_compute->CreateBuffer(g_bufferSize, ComputeBuf::BufferType::Map, "");

		// Create bind group layout
		std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(2, wgpu::Default);

		// Input buffer
		bindingLayoutEntries[0].binding = 0;
		bindingLayoutEntries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
		bindingLayoutEntries[0].visibility = wgpu::ShaderStage::Compute;

		// Output buffer
		bindingLayoutEntries[1].binding = 1;
		bindingLayoutEntries[1].buffer.type = wgpu::BufferBindingType::Storage;
		bindingLayoutEntries[1].visibility = wgpu::ShaderStage::Compute;

		m_compute->CreateBindGroup(bindingLayoutEntries);
		m_compute->CreatePipeline();

		m_inputBufferValues.resize(g_bufferSize / sizeof(float));
		for (int i = 0; i < m_inputBufferValues.size(); ++i) {
			m_inputBufferValues[i] = 0.1f * i;
		}
	}

	virtual void OnDetach() override
	{}

	virtual void OnUpdate(float ts) override
	{
        Walnut::Timer timer;

		m_compute->BeginComputePass();
		m_compute->SetBufferData(m_inputBufferValues.data(), g_bufferSize, "INPUT_BUFFER");
		m_compute->DoCompute(2, 1); // dispatch 2 workgroups to cover the entire buffer (one workgroup is 32 elements)
		m_compute->EndComputePass();

		auto& result = m_compute->GetMappedResult();
		assert(result.size() == g_bufferSize);
		const float* output = (const float*)(&result[0]);
		for (int i = 0; i < g_bufferSize / sizeof(float); ++i) {
			std::cout << "output " << output[i] << std::endl;
		}

        m_lastRenderTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", m_lastRenderTime);
		ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Results");
		ImGui::End();
        ImGui::PopStyleVar();

		ImGui::ShowDemoWindow();
	}

private:
    std::unique_ptr<Compute> m_compute;
    float m_lastRenderTime = 0.0f;
	std::vector<float> m_inputBufferValues;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Compute Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer2DLayer>();
	return app;
}