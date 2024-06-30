#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <Walnut/Image.h>

#include "../../../src/Compute.h"
#include <GLFW/glfw3.h>

constexpr int g_width = 1280;
constexpr int g_height = 720;

struct MyUniforms {
    float time;
};

class ComputeLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_finalImage = std::make_shared<Walnut::Image>(1, 1, Walnut::ImageFormat::RGBA);
		GPUInit();
		CPUInit();
	}

	virtual void OnDetach() override
	{}

	void GPUInit()
	{
		m_compute.reset();
		m_compute = std::make_unique<Compute>();

		const char* shaderSource = R"(

		struct MyUniforms {
			time: f32,
		};

		@group(0) @binding(0) var<storage,read> inputBuffer: array<f32>;
		@group(0) @binding(1) var<storage,read_write> outputBuffer: array<u32>;
		@group(0) @binding(2) var<uniform> uMyUniforms: MyUniforms;

		fn hash22(p: vec2u) -> vec2u {
			var v = p * 1664525u + 1013904223u;
			v.x += v.y * 1664525u; v.y += v.x * 1664525u;
			v ^= v >> vec2u(16u);
			v.x += v.y * 1664525u; v.y += v.x * 1664525u;
			v ^= v >> vec2u(16u);
			return v;
		}

		fn rand22(f: vec2f) -> vec2f { return vec2f(hash22(bitcast<vec2u>(f))) / f32(0xffffffff); }

		fn noise2(n: vec2f) -> f32 {
			let d = vec2f(0., 1.);
			let b = floor(n);
			let f = smoothstep(vec2f(0.), vec2f(1.), fract(n));

			let mix1 = mix(rand22(b), rand22(b + d.yx), f.x);
			let mix2 = mix(rand22(b + d.xx), rand22(b + d.yy), f.x);
			let finalmix = mix(mix1, mix2, f.y);
			return (finalmix.x + finalmix.y)/2;
		}

		@compute @workgroup_size(8,8)
		fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
			let blue = u32(noise2(vec2f(f32(id.x)+uMyUniforms.time*2000, f32(id.y)+uMyUniforms.time*6000)) * 500.);
			let green = u32(noise2(vec2f(f32(id.x)+uMyUniforms.time*4000, f32(id.y)+uMyUniforms.time*4000)) * 500.);
			let red = u32(noise2(vec2f(f32(id.x)+uMyUniforms.time*6000, f32(id.y)+uMyUniforms.time*2000)) * 500.);
			let result = (255 << 24) | (blue << 16) | (green << 8) | red;
			let arrayPos = &outputBuffer[id.x + (1280 * id.y)];
			*arrayPos = result;
		}
		)";
		m_compute->SetShader(shaderSource);

		constexpr uint32_t computeWidth = g_width;
		constexpr uint32_t computeHeight = g_height;
		const auto bufferSize = computeWidth * computeHeight * 4;
		m_compute->CreateBuffer(bufferSize, ComputeBuf::BufferType::Input, "INPUT_BUFFER");
		m_compute->CreateBuffer(bufferSize, ComputeBuf::BufferType::Output, "OUTPUT_BUFFER");
		m_compute->CreateBuffer(bufferSize, ComputeBuf::BufferType::Map, "");
		m_compute->CreateBuffer(1 * sizeof(MyUniforms), ComputeBuf::BufferType::Uniform, "UNIFORM_BUFFER");	

		// Create bind group layout
		std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(3, wgpu::Default);

		// Input buffer
		bindingLayoutEntries[0].binding = 0;
		bindingLayoutEntries[0].visibility = wgpu::ShaderStage::Compute;
		bindingLayoutEntries[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;

		// Output buffer
		bindingLayoutEntries[1].binding = 1;
		bindingLayoutEntries[1].visibility = wgpu::ShaderStage::Compute;
		bindingLayoutEntries[1].buffer.type = wgpu::BufferBindingType::Storage;

		// Uniform buffer
		bindingLayoutEntries[2].binding = 2;
		bindingLayoutEntries[2].visibility = wgpu::ShaderStage::Compute;
		bindingLayoutEntries[2].buffer.type = wgpu::BufferBindingType::Uniform;
		bindingLayoutEntries[2].buffer.minBindingSize = sizeof(MyUniforms);
		bindingLayoutEntries[2].buffer.hasDynamicOffset = false;

		m_compute->CreateBindGroup(bindingLayoutEntries);
		m_compute->CreatePipeline();

		m_inputBufferValues.resize(bufferSize / sizeof(float));
		for (int i = 0; i < m_inputBufferValues.size(); ++i) {
			m_inputBufferValues[i] = 0.1f * i;
		}
	}

	void GPUSolve()
	{
		m_compute->BeginComputePass();

		const auto bufferSize = m_inputBufferValues.size() * sizeof(float);
		m_compute->SetBufferData(m_inputBufferValues.data(), bufferSize, "INPUT_BUFFER");


		m_myUniformData.time = static_cast<float>(glfwGetTime());
		m_compute->SetBufferData(&m_myUniformData, 1 * sizeof(MyUniforms), "UNIFORM_BUFFER");

		// divide by workgroup size and divide by sizeof float as we iterate a float array in the shader
		const uint32_t workgroupDispatchCount = std::ceil(bufferSize / sizeof(float) / 64.0);
		m_compute->DoCompute(g_width/8, g_height/8); 
		m_compute->EndComputePass();

		auto& result = m_compute->GetMappedResult();
		assert(result.size() == bufferSize);
		// const uint32_t* output = (const uint32_t*)(&result[0]);
		// std::cout << "output " << std::endl;
		// for (int i = 0; i < bufferSize / sizeof(float); ++i) {
		// 	std::cout << output[i] << ", ";
		// }
		// std::cout << std::endl;


		m_finalImage->Resize(g_width, g_height);
		m_finalImageData.resize(m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4); // 4 for RGBA
		
		memcpy(m_finalImageData.data(), result.data(), m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4);

		m_finalImage->SetData(m_finalImageData.data());
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
		float viewHeight = m_viewportHeight;
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
	MyUniforms m_myUniformData;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Compute Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ComputeLayer>();
	return app;
}