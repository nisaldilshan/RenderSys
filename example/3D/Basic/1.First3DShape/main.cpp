#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <GLFW/glfw3.h>

/**
 * The same structure as in the shader, replicated in C++
 */
struct MyUniforms {
	// offset = 0 * sizeof(vec4f) -> OK
	std::array<float, 4> color;

	// offset = 16 = 4 * sizeof(f32) -> OK
	float time;

	// Add padding to make sure the struct is host-shareable
	float _pad[3];
};
// Have the compiler check byte alignment
static_assert(sizeof(MyUniforms) % 16 == 0);

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
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
			m_renderer.reset();
			m_renderer = std::make_shared<RenderSys::Renderer3D>();

			m_renderer->Init();	
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

			const char* shaderSource = R"(
			/**
			 * A structure with fields labeled with vertex attribute locations can be used
			 * as input to the entry point of a shader.
			 */
			struct VertexInput {
				@location(0) position: vec3f,
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

			/**
			 * A structure holding the value of our uniforms
			 */
			struct MyUniforms {
				color: vec4f,
				time: f32,
			};

			// Instead of the simple uTime variable, our uniform variable is a struct
			@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				let ratio = 640.0 / 480.0;
				var offset = vec2f(0.0);

				let angle = uMyUniforms.time; // you can multiply it go rotate faster

				// Rotate the position around the X axis by "mixing" a bit of Y and Z in
				// the original Y and Z.
				let alpha = cos(angle);
				let beta = sin(angle);
				var position = vec3f(
					in.position.x,
					alpha * in.position.y + beta * in.position.z,
					alpha * in.position.z - beta * in.position.y,
				);
				out.position = vec4f(position.x, position.y * ratio, position.z * 0.5 + 0.5, 1.0);
				
				out.color = in.color;
				return out;
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				let color = in.color * uMyUniforms.color.rgb;
				// Gamma-correction
				let corrected_color = pow(color, vec3f(2.2));
				return vec4f(corrected_color, uMyUniforms.color.a);
			}
			)";
			m_renderer->SetShaderAsString(shaderSource);

			//
			std::vector<float> vertexData;
			std::vector<uint16_t> indexData;
			auto success = Geometry::load3DGeometry(RESOURCE_DIR "/webgpu.txt", vertexData, indexData, 3);
			if (!success) 
			{
				std::cerr << "Could not load geometry!" << std::endl;
				return;
			}
			//

			// Vertex fetch
			// We now have 2 attributes
			std::vector<RenderSys::VertexAttribute> vertexAttribs(2);

			// Position attribute
			vertexAttribs[0].shaderLocation = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// Color attribute
			vertexAttribs[1].shaderLocation = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3; // different type!
			vertexAttribs[1].offset = 3 * sizeof(float); // non null offset!

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			// stride
			vertexBufferLayout.arrayStride = 6 * sizeof(float);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;


			m_renderer->SetVertexBufferData(vertexData.data(), vertexData.size() * sizeof(float), vertexBufferLayout);
			m_renderer->SetIndexBufferData(indexData);

			// Create binding layout (don't forget to = Default)
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(1);
			RenderSys::BindGroupLayoutEntry& bGLayoutEntry = bindingLayoutEntries[0];
			bGLayoutEntry.setDefault();
			// The binding index as used in the @binding attribute in the shader
			bGLayoutEntry.binding = 0;
			// The stage that needs to access this resource
			bGLayoutEntry.visibility = static_cast<RenderSys::ShaderStage>
							(static_cast<uint32_t>(RenderSys::ShaderStage::Vertex) | static_cast<uint32_t>(RenderSys::ShaderStage::Fragment));
			bGLayoutEntry.buffer.type = RenderSys::BufferBindingType::Uniform;
			bGLayoutEntry.buffer.minBindingSize = sizeof(MyUniforms);
			// Make this binding dynamic so we can offset it between draw calls
			bGLayoutEntry.buffer.hasDynamicOffset = true;

			m_renderer->CreateUniformBuffer(2, UniformBuf::UniformType::ModelViewProjection, sizeof(MyUniforms), bGLayoutEntry.binding);
			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			// Upload first value
			m_uniformData.time = static_cast<float>(glfwGetTime()) * 0.95f; // glfwGetTime returns a double
			m_uniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
			m_renderer->SetUniformBufferData(UniformBuf::UniformType::ModelViewProjection, &m_uniformData, 0);

			// Upload second value
			m_uniformData.time = static_cast<float>(glfwGetTime()) * 1.05f; // glfwGetTime returns a double
			m_uniformData.color = { 1.0f, 1.0f, 1.0f, 0.7f };
			m_renderer->SetUniformBufferData(UniformBuf::UniformType::ModelViewProjection, &m_uniformData, 1);
			//                               				^^^^^^^^^^^^^ beware of the non-null offset!

			m_renderer->RenderIndexed(0);
			m_renderer->RenderIndexed(1);
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
    std::shared_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;

	MyUniforms m_uniformData;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer3DLayer>();
	return app;
}