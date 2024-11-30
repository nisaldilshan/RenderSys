#include <array>
#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Timer.h>
#include <Walnut/RenderingBackend.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Geometry.h>
#include <RenderSys/Texture.h>
#include <RenderSys/Camera.h>

struct VertexAttributes {
	glm::vec3 position;
	// Texture mapping attributes represent the local frame in which
	// normals sampled from the normal map are expressed.
	glm::vec3 tangent; // T = local X axis
	glm::vec3 bitangent; // B = local Y axis
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;
};

struct MyUniforms {
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
	float hardness = 32.0f;
	float kd = 1.0f;
	float ks = 0.3f;

	float _pad;
};
static_assert(sizeof(LightingUniforms) % 16 == 0);

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		bool success = Geometry::loadGeometryFromObjWithUV<VertexAttributes>(RESOURCE_DIR "/Meshes/cylinder.obj", m_vertexData);
		assert(success);
		Geometry::populateTextureFrameAttributes(m_vertexData);

		auto baseColorTexture = Texture::loadTexture(RESOURCE_DIR "/Textures/cobblestone_floor_08_diff_2k.jpg");
		assert(baseColorTexture && baseColorTexture->GetWidth() > 0 && baseColorTexture->GetHeight() > 0 && baseColorTexture->GetMipLevelCount() > 0);

		auto normalTexture = Texture::loadTexture(RESOURCE_DIR "/Textures/cobblestone_floor_08_nor_gl_2k.png");
		assert(normalTexture && normalTexture->GetWidth() > 0 && normalTexture->GetHeight() > 0 && normalTexture->GetMipLevelCount() > 0);

		m_renderer = std::make_unique<RenderSys::Renderer3D>();
		m_renderer->Init();

		if (Walnut::RenderingBackend::GetBackend() == Walnut::RenderingBackend::BACKEND::Vulkan)
		{
			const char* vertexShaderSource = R"(
				#version 460
				layout(binding = 0) uniform UniformBufferObject {
					mat4 projectionMatrix;
					mat4 viewMatrix;
					mat4 modelMatrix;
					vec4 color;
					vec3 cameraWorldPosition;
					float time;
				} ubo;
				layout (location = 0) in vec3 aPos;
				layout (location = 1) in vec3 in_tangent;
				layout (location = 2) in vec3 in_bitangent;
				layout (location = 3) in vec3 in_normal;
				layout (location = 4) in vec3 in_color;
				layout (location = 5) in vec2 in_uv;

				layout (location = 0) out vec3 out_color;
				layout (location = 1) out vec3 out_normal;
				layout (location = 2) out vec2 out_uv;
				layout (location = 3) out vec3 out_viewDirection;

				void main() 
				{
					vec4 worldPosition = ubo.modelMatrix * vec4(aPos, 1.0);
					gl_Position = ubo.projectionMatrix * ubo.viewMatrix * worldPosition;
					vec4 mult = ubo.modelMatrix * vec4(in_normal, 0.0);
					out_color = in_color;
					vec4 norm = ubo.modelMatrix * vec4(in_normal, 0.0);
					out_normal = norm.xyz;
					out_uv = in_uv;
					out_viewDirection = ubo.cameraWorldPosition - worldPosition.xyz;
				}
			)";
			RenderSys::Shader vertexShader("Vertex");
			vertexShader.type = RenderSys::ShaderType::SPIRV;
			vertexShader.shaderSrc = vertexShaderSource;
			vertexShader.stage = RenderSys::ShaderStage::Vertex;
			m_renderer->SetShader(vertexShader);

			const char* fragmentShaderSource = R"(
				#version 460

				layout(binding = 0) uniform UniformBufferObject {
					mat4 projectionMatrix;
					mat4 viewMatrix;
					mat4 modelMatrix;
					vec4 color;
					float time;
					float _pad[3];
				} ubo;

				layout(binding = 1) uniform texture2D tex;
				layout(binding = 2) uniform texture2D normal;
				layout(binding = 3) uniform sampler s;

				layout(binding = 4) uniform LightingUniforms {
					vec4 directions[2];
					vec4 colors[2];
					float hardness;
					float kd;
					float ks;
					float _pad;
				} lightingUbo;

				layout (location = 0) in vec3 in_color;
				layout (location = 1) in vec3 in_normal;
				layout (location = 2) in vec2 in_uv;
				layout (location = 3) in vec3 in_viewDirection;

				layout (location = 0) out vec4 out_color;

				void main()
				{
					vec3 N = normalize(in_normal);
					vec3 V = normalize(in_viewDirection);
					vec3 texColor = texture(sampler2D(tex, s), in_uv).rgb; 
					vec3 color = vec3(0.0);
					for (int i = 0; i < 2; i++)
					{
						vec3 lightColor = lightingUbo.colors[i].rgb;
						vec3 L = normalize(lightingUbo.directions[i].xyz);
						vec3 R = reflect(-L, N); // equivalent to 2.0 * dot(N, L) * N - L

						vec3 diffuse = max(0.0, dot(L, N)) * lightColor;
						float RoV = max(0.0, dot(R, V));
						float specular = pow(RoV, lightingUbo.hardness);

						vec3 ambient = vec3(0.05);
						color += texColor * lightingUbo.kd * diffuse + lightingUbo.ks * specular + ambient;
					}

					// Gamma-correction
					vec3 corrected_color = pow(color, vec3(2.2));
					out_color = vec4(corrected_color, ubo.color.a);
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
			struct VertexInput {
				@location(0) position: vec3f,
				@location(1) normal: vec3f,
				@location(2) color: vec3f,
				@location(3) uv: vec2f,
				@location(4) tangent: vec3f,
				@location(5) bitangent: vec3f,
			};

			struct VertexOutput {
				@builtin(position) position: vec4f,
				@location(0) color: vec3f,
				@location(1) normal: vec3f,
				@location(2) uv: vec2f,
				@location(3) viewDirection: vec3<f32>,
				@location(4) tangent: vec3f,
				@location(5) bitangent: vec3f,
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
			@group(0) @binding(2) var normalTexture: texture_2d<f32>;
			@group(0) @binding(3) var textureSampler: sampler;
			@group(0) @binding(4) var<uniform> uLighting: LightingUniforms;

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				let worldPosition = uMyUniforms.modelMatrix * vec4<f32>(in.position, 1.0);
				out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * worldPosition;
				out.tangent = (uMyUniforms.modelMatrix * vec4f(in.tangent, 0.0)).xyz;
				out.bitangent = (uMyUniforms.modelMatrix * vec4f(in.bitangent, 0.0)).xyz;
				out.normal = (uMyUniforms.modelMatrix * vec4f(in.normal, 0.0)).xyz;
				out.color = in.color;
				out.uv = in.uv;
				out.viewDirection = uMyUniforms.cameraWorldPosition - worldPosition.xyz;
				return out;
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				// Sample normal
				let normalMapStrength = 1.0; // could be a uniform
				let encodedN = textureSample(normalTexture, textureSampler, in.uv).rgb;
				let localN = encodedN * 2.0 - 1.0;
				// The TBN matrix converts directions from the local space to the world space
				let localToWorld = mat3x3f(
					normalize(in.tangent),
					normalize(in.bitangent),
					normalize(in.normal),
				);
				let worldN = localToWorld * localN;
				let N = normalize(mix(in.normal, worldN, normalMapStrength));

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

		m_renderer->CreateTextureSampler();
		m_renderer->CreateTexture(1, baseColorTexture->GetWidth(), baseColorTexture->GetHeight(), baseColorTexture->GetData(), baseColorTexture->GetMipLevelCount());
		m_renderer->CreateTexture(2, normalTexture->GetWidth(), normalTexture->GetHeight(), normalTexture->GetData(), normalTexture->GetMipLevelCount());

		m_camera = std::make_unique<Camera::PerspectiveCamera>(30.0f, 0.01f, 100.0f);
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

			std::vector<RenderSys::VertexAttribute> vertexAttribs(6);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// Normal attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

			// Color attribute
			vertexAttribs[2].location = 2;
			vertexAttribs[2].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[2].offset = offsetof(VertexAttributes, color);

			// UV attribute
			vertexAttribs[3].location = 3;
			vertexAttribs[3].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[3].offset = offsetof(VertexAttributes, uv);

			// Tangent attribute
			vertexAttribs[4].location = 4;
			vertexAttribs[4].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[4].offset = offsetof(VertexAttributes, tangent);

			// Bitangent attribute
			vertexAttribs[5].location = 5;
			vertexAttribs[5].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[5].offset = offsetof(VertexAttributes, bitangent);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;

			assert(m_vertexData.size() > 0);
			m_renderer->SetVertexBufferData(m_vertexData.data(), m_vertexData.size() * sizeof(VertexAttributes), vertexBufferLayout);

			// Create binding layouts
			std::vector<RenderSys::BindGroupLayoutEntry> bindingLayoutEntries(5);
			// The uniform buffer binding that we already had
			RenderSys::BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[0];
			uniformBindingLayout.setDefault();
			uniformBindingLayout.binding = 0;
			uniformBindingLayout.visibility = RenderSys::ShaderStage::VertexAndFragment;
			uniformBindingLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			uniformBindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
			uniformBindingLayout.buffer.hasDynamicOffset = false;

			// The texture binding
			RenderSys::BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
			textureBindingLayout.setDefault();
			textureBindingLayout.binding = 1;
			textureBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			textureBindingLayout.texture.sampleType = RenderSys::TextureSampleType::Float;
			textureBindingLayout.texture.viewDimension = RenderSys::TextureViewDimension::_2D;

			// The normal map binding
			RenderSys::BindGroupLayoutEntry& normalTextureBindingLayout = bindingLayoutEntries[2];
			normalTextureBindingLayout.setDefault();
			normalTextureBindingLayout.binding = 2;
			normalTextureBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			normalTextureBindingLayout.texture.sampleType = RenderSys::TextureSampleType::Float;
			normalTextureBindingLayout.texture.viewDimension = RenderSys::TextureViewDimension::_2D;

			// The sampler binding
			RenderSys::BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[3];
			samplerBindingLayout.setDefault();
			samplerBindingLayout.binding = 3;
			samplerBindingLayout.visibility = RenderSys::ShaderStage::Fragment;
			samplerBindingLayout.sampler.type = RenderSys::SamplerBindingType::Filtering;

			// Lighting Uniforms
			RenderSys::BindGroupLayoutEntry& lightingUniformLayout = bindingLayoutEntries[4];
			lightingUniformLayout.setDefault();
			lightingUniformLayout.binding = 4;
			lightingUniformLayout.visibility = RenderSys::ShaderStage::Fragment; // only Fragment is needed
			lightingUniformLayout.buffer.type = RenderSys::BufferBindingType::Uniform;
			lightingUniformLayout.buffer.minBindingSize = sizeof(LightingUniforms);

			m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
			m_renderer->CreateUniformBuffer(lightingUniformLayout.binding, sizeof(LightingUniforms), 1);

			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
			m_camera->SetViewportSize((float)m_viewportWidth, (float)m_viewportHeight);
        }

		if (m_renderer)
		{
			m_camera->OnUpdate();

			m_renderer->BeginRenderPass();

			m_myUniformData.viewMatrix = m_camera->GetViewMatrix();
			m_myUniformData.projectionMatrix = m_camera->GetProjectionMatrix();

			const float time = static_cast<float>(glfwGetTime());
			glm::mat4x4 M1(1.0);
			float angle = time;
			M1 = glm::rotate(M1, angle, glm::vec3(0.0, 0.0, 1.0));
			M1 = glm::translate(M1, glm::vec3(0.0, 0.0, 0.0));
			M1 = glm::scale(M1, glm::vec3(0.3f));
			m_myUniformData.modelMatrix = M1;

			m_myUniformData.color = { 0.0f, 1.0f, 0.4f, 1.0f };
			m_myUniformData.cameraWorldPosition = m_camera->GetPosition();
			m_myUniformData.time = time;
			m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);

			// Initial values
			m_lightingUniformData.directions[0] = { 0.5f, -0.9f, 0.1f, 0.0f };
			m_lightingUniformData.directions[1] = { 0.2f, 0.4f, 0.3f, 0.0f };
			m_lightingUniformData.colors[0] = { 1.0f, 0.9f, 0.6f, 1.0f };
			m_lightingUniformData.colors[1] = { 0.6f, 0.9f, 1.0f, 1.0f };
			m_renderer->SetUniformBufferData(4, &m_lightingUniformData, 0);
			m_renderer->BindResources();

			m_renderer->Render(0);
			m_renderer->EndRenderPass();
		}

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
    std::unique_ptr<RenderSys::Renderer3D> m_renderer;
    uint32_t m_viewportWidth = 0;
    uint32_t m_viewportHeight = 0;
    float m_lastRenderTime = 0.0f;
	glm::vec4 m_clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyUniforms m_myUniformData;
	LightingUniforms m_lightingUniformData;
	std::vector<VertexAttributes> m_vertexData;
	std::unique_ptr<Camera::PerspectiveCamera> m_camera;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<Renderer3DLayer>();
	return app;
}