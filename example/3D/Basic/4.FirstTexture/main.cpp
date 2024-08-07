#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <GLFW/glfw3.h>

/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */
struct VertexAttributes {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;
};

struct MyUniforms {
	// We add transform matrices
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    std::array<float, 4> color;
    float time;
    float _pad[3];
};

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{}

	virtual void OnDetach() override
	{}

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
			struct VertexInput {
				@location(0) position: vec3f,
				@location(1) normal: vec3f,
				@location(2) color: vec3f,
				@location(3) uv: vec2f,
			};

			struct VertexOutput {
				@builtin(position) position: vec4f,
				@location(0) color: vec3f,
				@location(1) normal: vec3f,
				@location(2) uv: vec2f,
			};

			/**
			 * A structure holding the value of our uniforms
			 */
			struct MyUniforms {
				projectionMatrix: mat4x4f,
				viewMatrix: mat4x4f,
				modelMatrix: mat4x4f,
				color: vec4f,
				time: f32,
			};

			@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

			// The texture binding
			@group(0) @binding(1) var gradientTexture: texture_2d<f32>;

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * vec4f(in.position, 1.0);
				// Forward the normal
				out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
				out.color = in.color;
				out.uv = in.uv;

				return out;
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				// We remap UV coords to actual texel coordinates
				let texelCoords = vec2i(in.uv * vec2f(textureDimensions(gradientTexture)));
    			let color = textureLoad(gradientTexture, texelCoords, 0).rgb;

				// Gamma-correction
				let corrected_color = pow(color, vec3f(2.2));
				return vec4f(corrected_color, uMyUniforms.color.a);
			}
			)";
			m_renderer->SetShaderAsString(shaderSource);

			//
			std::vector<VertexAttributes> vertexData;
			bool success = Geometry::loadGeometryFromObjWithUV<VertexAttributes>(RESOURCE_DIR "/Meshes/cube.obj", vertexData);
			if (!success) 
			{
				std::cerr << "Could not load geometry!" << std::endl;
				assert(false);
				return;
			}
			//

			std::vector<RenderSys::VertexAttribute> vertexAttribs(4);

			// Position attribute
			vertexAttribs[0].shaderLocation = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// Normal attribute
			vertexAttribs[1].shaderLocation = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

			// Color attribute
			vertexAttribs[2].shaderLocation = 2;
			vertexAttribs[2].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[2].offset = offsetof(VertexAttributes, color);

			// UV attribute
			vertexAttribs[3].shaderLocation = 3;
			vertexAttribs[3].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			// stride
			vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;


			m_renderer->SetVertexBufferData(vertexData.data(), vertexData.size() * sizeof(VertexAttributes), vertexBufferLayout);

			// Create binding layouts

			// Since we now have 2 bindings, we use a vector to store them
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(2);
			// The uniform buffer binding that we already had
			RenderSys::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
			uniformBindingLayout.setDefault();
			uniformBindingLayout.binding = 0;
			uniformBindingLayout.visibility = RenderSys::ShaderStage::VertexAndFragment;
			uniformBindingLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			uniformBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
			uniformBindingLayout.buffer.hasDynamicOffset = true;

			// The texture binding
			RenderSys::BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
			textureBindingLayout.setDefault();
			textureBindingLayout.binding = 1;
			textureBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			textureBindingLayout.texture.sampleType = RenderSys::TextureSampleType::Float;
			textureBindingLayout.texture.viewDimension = RenderSys::TextureViewDimension::_2D;

			m_renderer->CreateUniformBuffer(2, UniformBuf::UniformType::ModelViewProjection, sizeof(MyUniforms), uniformBindingLayout.binding);

			constexpr uint32_t texWidth = 256;
			constexpr uint32_t texHeight = 256;
			std::vector<uint8_t> pixels(4 * texWidth * texHeight);

			for (uint32_t i = 0; i < texWidth; ++i) {
				for (uint32_t j = 0; j < texHeight; ++j) {
					uint8_t *p = &pixels[4 * (j * texWidth + i)];
					p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
					p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
					p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
					p[3] = 255; // a
				}
			}
			m_renderer->CreateTexture(texWidth, texHeight, pixels.data(), 1);

			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
			
        }

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();
			const float time = static_cast<float>(glfwGetTime());
			constexpr float PI = 3.14159265358979323846f;		
			m_uniformData.viewMatrix = glm::lookAt(glm::vec3(-2.0f, -3.0f, 2.0f), glm::vec3(0.0f), glm::vec3(0, 0, 1));
			m_uniformData.projectionMatrix = glm::perspective(30 * (PI / 180), (float)(m_viewportWidth / m_viewportHeight), 0.01f, 100.0f);

			// Upload first value
			float angle1 = 2.0f;
			glm::mat4x4 M1(1.0);
			angle1 = time * 0.9f;
			M1 = glm::rotate(M1, angle1, glm::vec3(0.0, 0.0, 1.0));
			M1 = glm::translate(M1, glm::vec3(1.0, 0.0, 0.0));
			M1 = glm::scale(M1, glm::vec3(0.3f));
			m_uniformData.modelMatrix = M1;

			m_uniformData.time = time; // glfwGetTime returns a double
			m_uniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
			m_renderer->SetUniformBufferData(UniformBuf::UniformType::ModelViewProjection, &m_uniformData, 0);
			////

			// Upload second value
			glm::mat4x4 M2(1.0);
			angle1 = time * 1.1f;
			M2 = glm::rotate(M2, angle1, glm::vec3(0.0, 0.0, 1.0));
			M2 = glm::translate(M2, glm::vec3(0.5, 0.0, 0.0));
			M2 = glm::scale(M2, glm::vec3(0.3f));
			m_uniformData.modelMatrix = M2;

			m_uniformData.time = time; // glfwGetTime returns a double
			m_uniformData.color = { 1.0f, 1.0f, 1.0f, 0.7f };
			m_renderer->SetUniformBufferData(UniformBuf::UniformType::ModelViewProjection, &m_uniformData, 1);
			////                         				^^^^^^^^^^^^^ beware of the non-null offset!

			m_renderer->Render(0);
			m_renderer->Render(1);
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
	app->PushLayer<Renderer2DLayer>();
	return app;
}