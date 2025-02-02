#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <RenderSys/Compute.h>
#include <imgui.h>

constexpr uint32_t g_bufferSize = 64 * sizeof(float);

class ComputeLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_compute.reset();
		m_compute = std::make_unique<RenderSys::Compute>();

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

		RenderSys::Shader shader("Compute");
		shader.type = RenderSys::ShaderType::WGSL;
		shader.shaderSrc = shaderSource;
		shader.stage = RenderSys::ShaderStage::Compute;
		m_compute->SetShader(shader);

		
		m_compute->CreateBuffer(0, g_bufferSize, RenderSys::ComputeBuf::BufferType::Input);
		m_compute->CreateBuffer(1, g_bufferSize, RenderSys::ComputeBuf::BufferType::Output);

		// Create bind group layout
		std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(2);

		// Input buffer
		bindingLayoutEntries[0].setDefault();
		bindingLayoutEntries[0].binding = 0;
		bindingLayoutEntries[0].buffer.type = RenderSys::BufferBindingType::ReadOnlyStorage;
		bindingLayoutEntries[0].buffer.bufferName = "INPUT_BUFFER";
		bindingLayoutEntries[0].visibility = RenderSys::ShaderStage::Compute;

		// Output buffer
		bindingLayoutEntries[1].setDefault();
		bindingLayoutEntries[1].binding = 1;
		bindingLayoutEntries[1].buffer.type = RenderSys::BufferBindingType::Storage;
		bindingLayoutEntries[1].buffer.bufferName = "OUTPUT_BUFFER";
		bindingLayoutEntries[1].visibility = RenderSys::ShaderStage::Compute;

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
		m_compute->SetBufferData(0, m_inputBufferValues.data(), g_bufferSize);
		m_compute->DoCompute(2, 1); // dispatch 2 workgroups to cover the entire buffer (one workgroup is 32 elements)
		m_compute->EndComputePass();

		auto& result = m_compute->GetMappedResult(1);
		assert(result.size() == g_bufferSize);
		const float* output = (const float*)(&result[0]);
		for (int i = 0; i < g_bufferSize / sizeof(float); ++i) {
			std::cout << "output " << output[i] << std::endl;
		}

        m_lastComputeTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last Compute: %.3fms", m_lastComputeTime);
		ImGui::End();
	}

private:
    std::unique_ptr<RenderSys::Compute> m_compute;
    float m_lastComputeTime = 0.0f;
	std::vector<float> m_inputBufferValues;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Compute Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ComputeLayer>();
	return app;
}