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
				uint outputBuffer[];
			};

			layout(binding = 2) uniform MyUniforms {
				float time;
				vec2 resolution;
			} uMyUniforms;

			float rand(uint seed) {
				uint state = seed;
				state = state * 1664525u + 1013904223u;
				state = state ^ (state >> 16u);
				state = state * 2654435761u;
				state = state ^ (state >> 16u);
				return float(state) / float(0xFFFFFFFFu); // Normalize to 0.0 - 1.0
			}

			layout(local_size_x = 8, local_size_y = 8) in;

			void main() {
				vec3 id = gl_GlobalInvocationID;
				vec2 resol = uMyUniforms.resolution;
				float time = uMyUniforms.time;
				vec2 coord = vec2(id.x, id.y);

				uint seed = uint(coord.x) + uint(coord.y) * uint(resol.x) + uint(time * 1000.0);

				uint r = uint(fract(float(rand(seed * 1u + 0u)) * 48271.0) * 255.0);
				uint g = uint(fract(float(rand(seed * 179u + 1u)) * 92039.0) * 255.0);
				uint b = uint(fract(float(rand(seed * 347u + 2u)) * 255.0) * 255.0);

				uint result = (0xFFu << 24u) | (b << 16u) | (g << 8u) | r;

				uint arrayPos = uint(id.y + resol.x * id.x);
				outputBuffer[arrayPos] = result;
			}
			)";
	
			RenderSys::Shader shader("Compute", shaderSource);
			shader.type = RenderSys::ShaderType::SPIRV;
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
			@group(0) @binding(1) var<storage, read_write> outputBuffer: array<u32>;
			@group(0) @binding(2) var<uniform> uMyUniforms: MyUniforms;
	
			fn rand(seed: u32) -> f32 {
				var state = seed;
				state = state * 1664525u + 1013904223u;  // Multiplier and increment
				state = state ^ (state >> 16u);         // Bitwise XOR and shift
				state = state * 2654435761u;            // Another multiplication
				state = state ^ (state >> 16u);         // Another XOR and shift
				return f32(state) / f32(0xFFFFFFFFu);     // Normalize to 0.0 - 1.0
			}
	
			@compute @workgroup_size(8,8)
			fn computeStuff(@builtin(global_invocation_id) id: vec3<u32>) {
				let resol = uMyUniforms.resolution;
				let time = uMyUniforms.time;
				let coord = vec2u(id.x, id.y);

				// Incorporate time into the seed for variation
				let seed = coord.x + coord.y * u32(resol.x) + u32(time * 1000.0); // Adjust multiplier as needed

				let r = u32(fract(rand(seed * 1u + 0u) * 48271.0) * 255.0);
				let g = u32(fract(rand(seed * 179u + 1u) * 92039.0) * 255.0);
				let b = u32(fract(rand(seed * 347u + 2u) * 255.0) * 255.0);

				let result = (0xFFu << 24u) | (b << 16u) | (g << 8u) | r;

				let arrayPos = &outputBuffer[id.y + u32(resol.x) * id.x];
				*arrayPos = result;
			}
			)";
			
			RenderSys::Shader shader("Compute", shaderSource);
			shader.type = RenderSys::ShaderType::WGSL;
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

		memcpy(m_finalImageData.data(), result.data(), m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4);
		m_finalImage->SetData(m_finalImageData.data());
	}

	std::vector<uint32_t> cpuBuffer;
	void CPUInit()
	{

	}

	void CPUSolve()
	{
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

		memcpy(m_finalImageData.data(), cpuBuffer.data(), m_finalImage->GetWidth() * m_finalImage->GetHeight() * 4);
		m_finalImage->SetData(m_finalImageData.data());
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