#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Random.h>
#include <Walnut/Timer.h>
#include <Walnut/Image.h>
#include <Walnut/RenderingBackend.h>
#include <RenderSys/Compute.h>
#include <imgui.h>

int g_width = 0;
int g_height = 0;

struct alignas(16) MyUniforms {
    float time;
	glm::vec2 resolution;
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
		m_compute = std::make_unique<RenderSys::Compute>();
		m_compute->Init();

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			const char* shaderSource = R"(
			#version 450

			layout(std430, binding = 0) buffer InputBuffer {
				float inputBuffer[];
			};

			layout(std430, binding = 1) buffer OutputBuffer {
				float outputBuffer[];
			};

			float f(float x) {
				return 2.0 * x + 1.0;
			}

			layout(local_size_x = 32) in;

			void main() {
				uint id = gl_GlobalInvocationID.x;

				outputBuffer[id] = f(inputBuffer[id]);
			}
			)";
	
			RenderSys::Shader shader("Compute");
			shader.type = RenderSys::ShaderType::SPIRV;
			shader.shaderSrc = shaderSource;
			shader.stage = RenderSys::ShaderStage::Compute;
			m_compute->SetShader(shader);
		}
		else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
		{
			const char* shaderSource = R"(
	
			struct MyUniforms {
				time: f32,
				resolution: vec2f
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
				let resol = uMyUniforms.resolution;
	
				let arrayPos = &outputBuffer[(id.y) + (u32(resol.x) * (id.x))];
				*arrayPos = result;
			}
			)";
			
			RenderSys::Shader shader("Compute");
			shader.type = RenderSys::ShaderType::WGSL;
			shader.shaderSrc = shaderSource;
			shader.stage = RenderSys::ShaderStage::Compute;
			m_compute->SetShader(shader);
		}
		else
		{
			assert(false);
		}
	}

	void GPUSolve()
	{
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

		if (m_reset || m_viewportWidth != g_width || m_viewportHeight != g_height)
		{
			m_compute->Destroy();
			m_reset = false;
			g_width = m_viewportWidth;
			g_height = m_viewportHeight;
			const auto bufferSize = g_width * g_height * 4;
			m_compute->CreateBuffer(0, bufferSize, RenderSys::ComputeBuf::BufferType::Input);
			m_compute->CreateBuffer(1, bufferSize, RenderSys::ComputeBuf::BufferType::Output);
			m_compute->CreateBuffer(2, 1 * sizeof(MyUniforms), RenderSys::ComputeBuf::BufferType::Uniform);	

			// Create bind group layout
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(3);

			// Input buffer
			bindingLayoutEntries[0].setDefault();
			bindingLayoutEntries[0].binding = 0;
			bindingLayoutEntries[0].visibility = RenderSys::ShaderStage::Compute;
			bindingLayoutEntries[0].buffer.type = RenderSys::BufferBindingType::ReadOnlyStorage;
			bindingLayoutEntries[0].buffer.bufferName = "INPUT_BUFFER";
			

			// Output buffer
			bindingLayoutEntries[1].setDefault();
			bindingLayoutEntries[1].binding = 1;
			bindingLayoutEntries[1].visibility = RenderSys::ShaderStage::Compute;
			bindingLayoutEntries[1].buffer.type = RenderSys::BufferBindingType::Storage;
			bindingLayoutEntries[1].buffer.bufferName = "OUTPUT_BUFFER";

			// Uniform buffer
			bindingLayoutEntries[2].setDefault();
			bindingLayoutEntries[2].binding = 2;
			bindingLayoutEntries[2].visibility = RenderSys::ShaderStage::Compute;
			bindingLayoutEntries[2].buffer.type = RenderSys::BufferBindingType::Uniform;
			bindingLayoutEntries[2].buffer.minBindingSize = sizeof(MyUniforms);
			bindingLayoutEntries[2].buffer.hasDynamicOffset = false;
			bindingLayoutEntries[2].buffer.bufferName = "UNIFORM_BUFFER";

			m_compute->CreateBindGroup(bindingLayoutEntries);
			m_compute->CreatePipeline();

			m_inputBufferValues.resize(bufferSize);
			for (int i = 0; i < m_inputBufferValues.size(); ++i) {
				m_inputBufferValues[i] = 0;
			}

			m_compute->SetBufferData(0, m_inputBufferValues.data(), m_inputBufferValues.size());
		}

		m_compute->BeginComputePass();
		
		m_myUniformData.time = static_cast<float>(glfwGetTime());
		m_myUniformData.resolution.x = g_width;
		m_myUniformData.resolution.y = g_height;
		m_compute->SetBufferData(2, &m_myUniformData, 1 * sizeof(MyUniforms));

		constexpr auto workgroup_size_x = 8;
		constexpr auto workgroup_size_y = 8;
		const auto dispatch_x = (g_width + workgroup_size_x - 1) / workgroup_size_x;
		const auto dispatch_y = (g_height + workgroup_size_y - 1) / workgroup_size_y;
		m_compute->DoCompute(dispatch_x, dispatch_y); 
		m_compute->EndComputePass();

		auto& result = m_compute->GetMappedResult(1);
		assert(result.size() == m_inputBufferValues.size());

		m_finalImage->Resize(g_width, g_height);
		m_finalImageData.resize(m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4); // 4 for RGBA
		memcpy(m_finalImageData.data(), result.data(), m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4);
		m_finalImage->SetData(m_finalImageData.data());
	}

	std::vector<uint32_t> cpuBuffer;
	void CPUInit()
	{

	}

	void CPUSolve()
	{
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

		if (m_reset || m_viewportWidth != g_width || m_viewportHeight != g_height)
		{
			m_reset = false;
			g_width = m_viewportWidth;
			g_height = m_viewportHeight;
			const auto bufferSize = g_width * g_height;
			cpuBuffer.resize(bufferSize);
		}

		int counter = 0;
		for (auto &pixel : cpuBuffer)
		{
			pixel = Walnut::Random::UInt();
			pixel |= 0xff000000; // remove randomnes from alpha channel
		}

		m_finalImage->Resize(g_width, g_height);
		m_finalImageData.resize(m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4); // 4 for RGBA
		memcpy(m_finalImageData.data(), cpuBuffer.data(), m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4);
		m_finalImage->SetData(m_finalImageData.data());
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
		m_reset = ImGui::Checkbox("HW", &m_hWSolver);
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
    std::unique_ptr<RenderSys::Compute> m_compute;
    float m_lastRenderTime = 0.0f;
	std::vector<uint8_t> m_inputBufferValues;
	uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
	bool m_hWSolver = false;
	std::shared_ptr<Walnut::Image> m_finalImage = nullptr;
	std::vector<uint8_t> m_finalImageData;
	MyUniforms m_myUniformData;
	bool m_reset = true;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Compute Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ComputeLayer>();
	return app;
}