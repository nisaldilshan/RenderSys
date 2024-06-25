#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <Walnut/Image.h>

#include "../../../src/Compute.h"
#include <GLFW/glfw3.h>

constexpr uint32_t g_bufferSize = 64 * sizeof(float);

class ComputeLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_finalImage = std::make_shared<Walnut::Image>(1, 1, Walnut::ImageFormat::RGBA);
	}

	virtual void OnDetach() override
	{}

	void GPUInit()
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

	void GPUSolve()
	{
		m_compute->BeginComputePass();
		m_compute->SetBufferData(m_inputBufferValues.data(), g_bufferSize, "INPUT_BUFFER");
		m_compute->DoCompute();
		m_compute->EndComputePass();

		auto& result = m_compute->GetMappedResult();
		assert(result.size() == g_bufferSize);
		const float* output = (const float*)(&result[0]);
		for (int i = 0; i < g_bufferSize / sizeof(float); ++i) {
			std::cout << "output " << output[i] << std::endl;
		}
	}

	void CPUInit()
	{
		
	}

	void CPUSolve()
	{

	}

	virtual void OnUpdate(float ts) override
	{
		Walnut::Timer timer;
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

		if (m_viewportWidth != m_finalImage->GetWidth() || m_viewportHeight != m_finalImage->GetHeight())
		{
			m_finalImage->Resize(m_viewportWidth, m_viewportHeight);
			m_finalImageData.resize(m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4); // 4 for RGBA
			GPUInit();
			CPUInit();
		}

        if (m_hWSolver)
		{
			GPUSolve();
		}
		else
		{
			CPUSolve();
		}

        m_lastRenderTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", m_lastRenderTime);
		ImGui::Checkbox("HW", &m_hWSolver);
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
        m_viewportHeight = ImGui::GetContentRegionAvail().y;

		assert(m_finalImage);

		ImVec2 uv_min = ImVec2(1, 0);                 // Top-left
		ImVec2 uv_max = ImVec2(0, 1); 
		float aspectRatio = (float)m_finalImage->GetWidth() / (float)m_finalImage->GetHeight();
		float viewHeight = (float)m_finalImage->GetHeight();
		ImGui::Image((void*)m_finalImage->GetDescriptorSet(), { aspectRatio * viewHeight, viewHeight }, uv_min, uv_max);

		ImGui::End();
        ImGui::PopStyleVar();
	}

private:
    std::unique_ptr<Compute> m_compute;
    float m_lastRenderTime = 0.0f;
	std::vector<float> m_inputBufferValues;
	uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
	bool m_hWSolver = false;
	std::shared_ptr<Walnut::Image> m_finalImage = nullptr;
	std::vector<uint8_t> m_finalImageData;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Compute Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ComputeLayer>();
	return app;
}