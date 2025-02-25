#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <Walnut/Image.h>
#include <RenderSys/Renderer2D.h>
#include <imgui.h>

#include "FluidSolver2D.h"
#include "Utils.h"


/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */
struct VertexAttributes {
	glm::vec2 position;
	glm::vec2 uv;
};

struct MyUniforms {
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

constexpr uint32_t simulationDimension = 512;

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer.reset();
		m_renderer = std::make_shared<RenderSys::Renderer2D>();

		const char* shaderSource = R"(
		struct VertexInput {
			@location(0) position: vec2f,
			@location(1) uv: vec2f,
		};

		struct VertexOutput {
			@builtin(position) position: vec4f,
			@location(0) uv: vec2f,
		};

		struct MyUniforms {
			cameraWorldPosition: vec3f,
			time: f32,
		};

		@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

		@vertex
		fn vs_main(in: VertexInput) -> VertexOutput {
			var out: VertexOutput;
			out.position = vec4f(in.position, 0.0, 1.0);
			out.uv = in.uv;
			return out;
		}

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

		@fragment
		fn fs_main(in: VertexOutput) -> @location(0) vec4f {
			let noise_1 = noise2(vec2f(in.position.x + uMyUniforms.time*10000, in.position.y + uMyUniforms.time*30000));
			let noise_2 = noise2(vec2f(in.position.x + uMyUniforms.time*20000, in.position.y + uMyUniforms.time*10000));
			let noise_3 = noise2(vec2f(in.position.x + uMyUniforms.time*30000, in.position.y + uMyUniforms.time*20000));
			return vec4f(noise_1, noise_2, noise_3, 1.0);
		}

		)";
		RenderSys::Shader shader("Combined");
		shader.type = RenderSys::ShaderType::WGSL;
		shader.shaderSrc = shaderSource;
		shader.stage = RenderSys::ShaderStage::VertexAndFragment;
		m_renderer->SetShader(shader);

		m_finalImage = std::make_shared<Walnut::Image>(1, 1, Walnut::ImageFormat::RGBA);
		m_fluid = std::make_unique<FluidPlane>(simulationDimension);
		m_solver = std::make_unique<FluidSolver2D>(*m_fluid);
	}

	virtual void OnDetach() override
	{

	}

	void GPUSolve()
	{
		// uint32_t renderWidth = m_viewportWidth;
		// uint32_t renderHeight = m_viewportHeight;
		uint32_t renderWidth = simulationDimension;
		uint32_t renderHeight = simulationDimension;

		if (!m_renderer ||
            renderWidth != m_renderer->GetWidth() ||
            renderHeight != m_renderer->GetHeight())
        {
			m_renderer->Init();
			m_renderer->OnResize(renderWidth, renderHeight);

			// Vertex buffer
			// There are 2 floats per vertex, one for x and one for y.
			// But in the end this is just a bunch of floats to the eyes of the GPU,
			// the *layout* will tell how to interpret this.
			const std::vector<float> vertexData = {
				-0.999, -0.999, -1.0 , -1.0,
				+0.999, -0.999, 1.0 , -1.0,
				+0.999, +0.999, 1.0 , 1.0,

				-0.999, -0.999, -1.0 , -1.0,
				-0.999, +0.999, -1.0 , 1.0,
				+0.999, +0.999, 1.0 , 1.0,
			};

			std::vector<RenderSys::VertexAttribute> vertexAttribs(2);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[0].offset = 0;

			// UV attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[1].offset = offsetof(VertexAttributes, uv);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;


			m_renderer->SetVertexBufferData(vertexData.data(), vertexData.size() * 4, vertexBufferLayout);
			// Create binding layout (don't forget to = Default)
			// RenderSys::BindGroupLayoutEntry bGLayoutEntry;
			// bGLayoutEntry.setDefault();
			// // The binding index as used in the @binding attribute in the shader
			// bGLayoutEntry.binding = 0;
			// // The stage that needs to access this resource
			// bGLayoutEntry.visibility = RenderSys::ShaderStage::Vertex;
			// bGLayoutEntry.buffer.type = RenderSys::BufferBindingType::Uniform;
			// bGLayoutEntry.buffer.minBindingSize = sizeof(MyUniforms);

			// Create binding layouts
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(1);
			// The uniform buffer binding that we already had
			RenderSys::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
			uniformBindingLayout.setDefault();
			uniformBindingLayout.binding = 0;
			uniformBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			uniformBindingLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			uniformBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
			uniformBindingLayout.buffer.hasDynamicOffset = false;

			m_renderer->CreateUniformBuffer(1, sizeof(MyUniforms));			
			m_renderer->CreateBindGroup(uniformBindingLayout);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			const float time = static_cast<float>(glfwGetTime());
			m_myUniformData.time = time;
			m_renderer->SetUniformBufferData(&m_myUniformData, 0);
       		m_renderer->Render();
			m_renderer->EndRenderPass();
		}
	}

	void CPUSolve()
	{
		// uint32_t renderWidth = m_viewportWidth;
		// uint32_t renderHeight = m_viewportHeight;
		uint32_t renderWidth = simulationDimension;
		uint32_t renderHeight = simulationDimension;
		if (!m_imageData)
		{
			m_imageData = new uint32_t[renderWidth * renderHeight];
			m_finalImage->Resize(renderWidth, renderHeight);
		}

		if (renderWidth != m_finalImage->GetWidth() || renderHeight != m_finalImage->GetHeight())
		{
			delete[] m_imageData;
			m_imageData = new uint32_t[renderWidth * renderWidth];
			m_finalImage->Resize(renderWidth, renderWidth);
		}

		for (int i = -1; i <= 1; i++) {
    		for (int j = -1; j <= 1; j++) {
				uint32_t some = Walnut::Random::UInt();
				auto amount = float(some % 500)+ 100.0f;
				m_solver->FluidPlaneAddDensity(renderWidth/2 + i, renderWidth/2 + j, amount);
			}
		}

		static glm::vec2 angle = glm::circularRand(10.0f);
		static std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		if (currentTime - lastTime > std::chrono::milliseconds(2000))
		{
			angle = glm::circularRand(20.0f);
			lastTime = currentTime;
		}

		for (int i = 0; i < 2; i++) {
			m_solver->FluidPlaneAddVelocity(renderWidth/2, renderWidth/2, angle.x, angle.y);
		}

		m_solver->FluidSolveStep();

		for (size_t i = 0; i < renderWidth * renderWidth; i++)
		{
			const float density = m_fluid->density[i];
			if (density > m_desityMax)
			{
				m_desityMax = density;
			}

			const glm::vec2 velocity = glm::vec2(m_fluid->Vx[i], m_fluid->Vy[i]);
			if (glm::length(velocity) > glm::length(m_velocityMax))
			{
				m_velocityMax = velocity;
			}

			float hue = fmodf(density + 50.0f, 360.0f); // (d + 50) % 360
			float saturation = 1.0f;
			float value = fmodf(density, m_desityMax); // d

			float red = 0.0f;
			float green = 0.0f;
			float blue = 0.0f;

			Utils::HSVtoRGB(red, green, blue, hue, saturation, value);

			uint8_t redInt = static_cast<uint8_t>(red);
			uint8_t greenInt = static_cast<uint8_t>(green);
			uint8_t blueInt = static_cast<uint8_t>(blue);

			
			constexpr uint8_t alpha = 255;
        	uint32_t result = (alpha << 24) | (blueInt << 16) | (greenInt << 8) | redInt;
			m_imageData[i] = result;
		}

		m_finalImage->SetData(m_imageData);
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
		ImGui::Text("Max Density: %.3fms", m_desityMax);
		ImGui::Text("Max Velocity: %.3fms", glm::length(m_velocityMax));
		ImGui::Text("Optimum Delta Time: %.6fms", (1.0f / simulationDimension) / glm::length(m_velocityMax));


		ImGui::Checkbox("HW", &m_hWSolver);

		ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
        m_viewportHeight = ImGui::GetContentRegionAvail().y;

		if (m_hWSolver)
		{
			if (m_renderer)
            	ImGui::Image(m_renderer->GetDescriptorSet(), {(float)m_renderer->GetWidth(),(float)m_renderer->GetWidth()});
			else
				assert(false);
		}
		else
		{
			if (m_finalImage)
			{
				ImVec2 uv_min = ImVec2(1, 0);                 // Top-left
				ImVec2 uv_max = ImVec2(0, 1); 
				float aspectRatio = (float)m_finalImage->GetWidth() / (float)m_finalImage->GetHeight();
				float viewHeight = (float)m_finalImage->GetHeight();
				ImGui::Image((void*)m_finalImage->GetDescriptorSet(), { aspectRatio * viewHeight, viewHeight }, uv_min, uv_max);
			}
			else
				assert(false);
		}
        
		ImGui::End();
        ImGui::PopStyleVar();

		ImGui::ShowDemoWindow();
	}

private:
    std::shared_ptr<RenderSys::Renderer2D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	MyUniforms m_myUniformData;

	bool m_hWSolver = false;
	uint32_t* m_imageData = nullptr;
	std::shared_ptr<Walnut::Image> m_finalImage = nullptr;
	std::unique_ptr<FluidPlane> m_fluid;
	std::unique_ptr<FluidSolver2D> m_solver;

	float m_desityMax = 0.0f;
	glm::vec2 m_velocityMax = glm::vec2(0.0f);
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer2DLayer>();
	return app;
}