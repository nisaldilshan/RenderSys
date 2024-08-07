#define DAWN_DEBUG_BREAK_ON_ERROR 1

#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <RenderSys/Texture.h>
#include <RenderSys/Camera.h>
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
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

struct LightingUniforms {
    std::array<glm::vec4, 2> directions;
    std::array<glm::vec4, 2> colors;
	// Material properties
	float hardness = 16.0f;
	float kd = 1.5f;
	float ks = 0.5f;

	float _pad[1];
};
static_assert(sizeof(LightingUniforms) % 16 == 0);

class Renderer2DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		bool success = Geometry::loadGeometryFromObjWithUV<VertexAttributes>(RESOURCE_DIR "/Meshes/fourareen.obj", m_vertexData);
		if (!success) 
		{
			std::cerr << "Could not load geometry!" << std::endl;
			assert(false);
		}

		m_texHandle = Texture::loadTexture(RESOURCE_DIR "/Textures/fourareen2K_albedo.jpg");
		assert(m_texHandle && m_texHandle->GetWidth() > 0 && m_texHandle->GetHeight() > 0 && m_texHandle->GetMipLevelCount() > 0);

		m_shaderSource = R"(
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
			@location(3) viewDirection: vec3<f32>,
		};

		/**
		 * A structure holding the value of our uniforms
		 */
		struct MyUniforms {
			projectionMatrix: mat4x4f,
			viewMatrix: mat4x4f,
			modelMatrix: mat4x4f,
			color: vec4f,
			cameraWorldPosition: vec3f,
			time: f32,
		};

		/**
		 * A structure holding the lighting settings
		 */
		struct LightingUniforms {
			directions: array<vec4f, 2>,
			colors: array<vec4f, 2>,
			hardness: f32,
			kd: f32,
			ks: f32,
		}

		@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;
		@group(0) @binding(1) var baseColorTexture: texture_2d<f32>;
		@group(0) @binding(2) var textureSampler: sampler;
		@group(0) @binding(3) var<uniform> uLighting: LightingUniforms;

		@vertex
		fn vs_main(in: VertexInput) -> VertexOutput {
			var out: VertexOutput;
			let worldPosition = uMyUniforms.modelMatrix * vec4<f32>(in.position, 1.0);
			out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * worldPosition;
			out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
			out.color = in.color;
			out.uv = in.uv;
			out.viewDirection = uMyUniforms.cameraWorldPosition - worldPosition.xyz;
			return out;
		}

		@fragment
		fn fs_main(in: VertexOutput) -> @location(0) vec4f {
			// Compute shading
			let N = normalize(in.normal);
			let V = normalize(in.viewDirection);

			// Sample texture
			let baseColor = textureSample(baseColorTexture, textureSampler, in.uv).rgb;
			let kd = uLighting.kd;
			let ks = uLighting.ks;
			let hardness = uLighting.hardness;

			var color = vec3f(0.0);
			for (var i: i32 = 0; i < 2; i++) {
				let lightColor = uLighting.colors[i].rgb;
				let L = normalize(uLighting.directions[i].xyz);
				let R = reflect(-L, N); // equivalent to 2.0 * dot(N, L) * N - L

				let diffuse = max(0.0, dot(L, N)) * lightColor;

				// We clamp the dot product to 0 when it is negative
				let RoV = max(0.0, dot(R, V));
				let specular = pow(RoV, hardness);

				color += baseColor * kd * diffuse + ks * specular;
			}
			
			// Gamma-correction
			let corrected_color = pow(color, vec3f(2.2));
			return vec4f(corrected_color, uMyUniforms.color.a);
		}
		)";

		m_renderer.reset();
		m_renderer = std::make_unique<RenderSys::Renderer3D>();

		assert(m_shaderSource);
		m_renderer->SetShaderAsString(m_shaderSource);

		m_renderer->CreateTextureSampler();
		m_renderer->CreateTexture(m_texHandle->GetWidth(), m_texHandle->GetHeight(), m_texHandle->GetData(), m_texHandle->GetMipLevelCount());

		m_camera = std::make_unique<Camera::PerspectiveCamera>(30.0f, 0.01f, 100.0f);
	}

	virtual void OnDetach() override
	{}

	virtual void OnUpdate(float ts) override
	{
        Walnut::Timer timer;
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return;

        if (m_viewportWidth != m_renderer->GetWidth() || m_viewportHeight != m_renderer->GetHeight())
        {
			InitialCode();
			m_camera->SetViewportSize((float)m_viewportWidth, (float)m_viewportHeight);
        }

		m_camera->OnUpdate();

		m_renderer->BeginRenderPass();

		m_myUniformData.viewMatrix = m_camera->GetViewMatrix();
		m_myUniformData.projectionMatrix = m_camera->GetProjectionMatrix();

		float angle1 = 2.0f;
		const float time = static_cast<float>(glfwGetTime());	
		glm::mat4x4 M1(1.0);
		angle1 = time * 0.9f;
		M1 = glm::rotate(M1, angle1, glm::vec3(0.0, 0.0, 1.0));
		M1 = glm::translate(M1, glm::vec3(0.0, 0.0, 0.0));
		M1 = glm::scale(M1, glm::vec3(0.3f));
		m_myUniformData.modelMatrix = M1;

		m_myUniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
		m_myUniformData.cameraWorldPosition = m_camera->GetPosition();
		m_myUniformData.time = time;
		m_renderer->SetUniformBufferData(UniformBuf::UniformType::ModelViewProjection, &m_myUniformData, 0);

		// Initial values
		m_lightingUniformData.directions[0] = { 0.5f, 0.5f, 0.5f, 0.0f };
		m_lightingUniformData.directions[1] = { -0.5f, -0.5f, -0.5f, 0.0f };
		m_lightingUniformData.colors[0] = { 1.0f, 0.9f, 0.6f, 1.0f };
		m_lightingUniformData.colors[1] = { 0.6f, 0.9f, 1.0f, 1.0f };
		m_renderer->SetUniformBufferData(UniformBuf::UniformType::Lighting, &m_lightingUniformData, 0);

		m_renderer->Render(0);

		m_renderer->EndRenderPass();

        m_lastRenderTime = timer.ElapsedMillis();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", m_lastRenderTime);
		static ImVec4 newClearColorImgui = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImGui::ColorEdit3("Clear Color", (float*)&newClearColorImgui); 
		glm::vec4 newClearColor = {newClearColorImgui.x, newClearColorImgui.y, newClearColorImgui.z, newClearColorImgui.w};
		if (newClearColor != m_clearColor)
		{
			m_clearColor = newClearColor;
			m_renderer->SetClearColor(m_clearColor);
		}
		ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
		m_viewportWidth = ImGui::GetContentRegionAvail().x;
        m_viewportHeight = ImGui::GetContentRegionAvail().y;
        ImGui::Image(m_renderer->GetDescriptorSet(), {(float)m_renderer->GetWidth(),(float)m_renderer->GetWidth()});
		ImGui::End();
        ImGui::PopStyleVar();

		ImGui::ShowDemoWindow();
	}

private:
	void InitialCode()
	{
		m_renderer->Init();
		m_renderer->OnResize(m_viewportWidth, m_viewportHeight);

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

		assert(m_vertexData.size() > 0);
		m_renderer->SetVertexBufferData(m_vertexData.data(), m_vertexData.size() * sizeof(VertexAttributes), vertexBufferLayout);

		// Create binding layouts

		// Since we now have 2 bindings, we use a vector to store them
		std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(4);
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

		// The sampler binding
		RenderSys::BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
		samplerBindingLayout.setDefault();
		samplerBindingLayout.binding = 2;
		samplerBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
		samplerBindingLayout.sampler.type = RenderSys::SamplerBindingType::Filtering;

		// Lighting Uniforms
		RenderSys::BindGroupLayoutEntry& lightingUniformLayout = bindingLayoutEntries[3];
		lightingUniformLayout.setDefault();
		lightingUniformLayout.binding = 3;
		lightingUniformLayout.visibility = RenderSys::ShaderStage::Fragment; // only Fragment is needed
		lightingUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
		lightingUniformLayout.buffer.minBindingSize = sizeof(LightingUniforms);

		m_renderer->CreateUniformBuffer(1, UniformBuf::UniformType::ModelViewProjection, sizeof(MyUniforms), uniformBindingLayout.binding);
		m_renderer->CreateUniformBuffer(1, UniformBuf::UniformType::Lighting, sizeof(LightingUniforms), lightingUniformLayout.binding);

		m_renderer->CreateBindGroup(bindingLayoutEntries);

		m_renderer->CreatePipeline();
	}

    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	glm::vec4 m_clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyUniforms m_myUniformData;
	LightingUniforms m_lightingUniformData;
	std::vector<VertexAttributes> m_vertexData;
	std::unique_ptr<Texture::TextureHandle> m_texHandle = nullptr;
	const char* m_shaderSource = nullptr;

	std::unique_ptr<Camera::PerspectiveCamera> m_camera;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer2DLayer>();
	return app;
}