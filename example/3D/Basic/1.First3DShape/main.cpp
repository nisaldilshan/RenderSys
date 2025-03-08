#include <array>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <imgui.h>

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
		m_renderer = std::make_shared<RenderSys::Renderer3D>();
		m_renderer->Init();	

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			const char* vertexShaderSource = R"(
				#version 450 core
				layout(binding = 0) uniform UniformBufferObject {
					vec4 color;
					float time;
					float _pad[3];
				} ubo;
				layout (location = 0) in vec3 aPos;
				layout (location = 1) in vec3 aColor; // Add color attribute
				layout (location = 0) out vec3 vColor; // Add color attribute

				void main() 
				{
					float angle = ubo.time;
					float alpha = cos(angle);
					float beta = sin(angle);
					vec3 pos = vec3(
						aPos.x,
						alpha * aPos.y + beta * aPos.z,
						alpha * aPos.z - beta * aPos.y
					);
					gl_Position = vec4(pos.x, pos.y, pos.z*0.5 + 0.5, 1.0);
					vColor = aColor; // Pass color to fragment shader
				}
			)";
			RenderSys::Shader vertexShader("Vertex");
			vertexShader.type = RenderSys::ShaderType::SPIRV;
			vertexShader.shaderSrc = vertexShaderSource;
			vertexShader.stage = RenderSys::ShaderStage::Vertex;
			m_renderer->SetShader(vertexShader);

			const char* fragmentShaderSource = R"(
				#version 450
				layout(binding = 0) uniform UniformBufferObject {
					vec4 color;
					float time;
					float _pad[3];
				} ubo;
				layout(location = 0) in vec3 vColor;
				layout(location = 0) out vec4 FragColor;

				void main()
				{
					FragColor = vec4(vColor, 1.0) * ubo.color;
				}
			)";
			RenderSys::Shader fragmentShader("Fragment");
			fragmentShader.type = RenderSys::ShaderType::SPIRV;
			fragmentShader.shaderSrc = fragmentShaderSource;
			fragmentShader.stage = RenderSys::ShaderStage::Fragment;
			m_renderer->SetShader(fragmentShader);
		}
		else if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::WebGPU)
		{
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
				let angle = uMyUniforms.time; // you can multiply it go rotate faster

				// Rotate the position around the X axis by "mixing" a bit of Y and Z in
				// the original Y and Z.
				let alpha = cos(angle);
				let beta = sin(angle);
				var position = vec3f(
					in.position.x,
					alpha * in.position.y + beta * in.position.z,
					alpha * in.position.z - beta * in.position.y
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

			RenderSys::Shader shader("Combined");
			shader.type = RenderSys::ShaderType::WGSL;
			shader.shaderSrc = shaderSource;
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);
		}
		else
		{
			assert(false);
		}

		RenderSys::VertexBuffer vertexData;
		std::vector<uint32_t> indexData;
		auto success = Geometry::load3DGeometry(RESOURCE_DIR "/model.txt", vertexData, indexData, 3, true);
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
		vertexAttribs[0].location = 0;
		vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
		vertexAttribs[0].offset = 0;

		// Color attribute
		vertexAttribs[1].location = 1;
		vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3; 
		vertexAttribs[1].offset = offsetof(RenderSys::Vertex, color);

		RenderSys::VertexBufferLayout vertexBufferLayout;
		vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
		vertexBufferLayout.attributes = vertexAttribs.data();
		vertexBufferLayout.arrayStride = sizeof(RenderSys::Vertex);
		vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

		auto vertexBufID = m_renderer->SetVertexBufferData(vertexData, vertexBufferLayout);
		m_renderer->SetIndexBufferData(vertexBufID, indexData);
	}

	virtual void OnDetach() override
	{
		m_renderer->Destroy();
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
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

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
			bGLayoutEntry.buffer.hasDynamicOffset = false;

			m_renderer->CreateUniformBuffer(bGLayoutEntry.binding, sizeof(MyUniforms), 2);
			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			m_uniformData.time = static_cast<float>(glfwGetTime()) * 0.95f; // glfwGetTime returns a double
			m_uniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
			m_renderer->SetUniformBufferData(0, &m_uniformData, 0);

			m_renderer->BindResources();
			
			m_renderer->RenderIndexed(0);
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