#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Random.h"
#include <Walnut/Timer.h>
#include <GLFW/glfw3.h>

#include <RenderSys/Renderer3D.h>
#include <RenderSys/Camera.h>
#include <imgui.h>

struct MyUniforms {
	glm::vec3 cameraWorldPosition;
    float time;
};
static_assert(sizeof(MyUniforms) % 16 == 0);

class Renderer3DLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_renderer.reset();
		m_renderer = std::make_shared<RenderSys::Renderer3D>();
		m_camera = std::make_unique<Camera::PerspectiveCamera>(30.0f, 0.01f, 100.0f);
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
			m_renderer->Init();
			m_renderer->OnResize(m_viewportWidth, m_viewportHeight);
			m_camera->SetViewportSize((float)m_viewportWidth, (float)m_viewportHeight);

			const char* shaderSource = R"(
			
			struct VertexInput {
				@location(0) position: vec3f,
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

			const MAX_DIST = 100.0; 

			fn sdSphere(currentRayPosition: vec3f) -> f32
			{
				let radius = 0.5;
				let pos = vec3f(uMyUniforms.time/10, 0.0, 0.0);
  				return length(currentRayPosition - pos) - radius;
			}

			fn sdCube(currentRayPosition: vec3f) -> f32
			{
				let size = 0.5;
				let q = abs(currentRayPosition) - size;
  				return length(max(q, vec3f(0.0))) + min(max(q.x, max(q.y, q.z)), 0.0);
			}

			fn map(currentRayPosition: vec3f) -> f32 
			{
				let sphere = sdSphere(currentRayPosition);
				let cube = sdCube(currentRayPosition);
				return min(sphere, cube);
			}

			fn rayMarch(rayDirection: vec3f, cameraPos: vec3f) -> f32
			{
				var t = 0.0; // total distance travelled form cameras ray origin
				for (var i = 0; i < 80; i++)
				{
					let p = cameraPos + rayDirection * t;
					let d = map(p);
					t = t + d;

					if d < 0.001 || t > MAX_DIST
					{
						break;
					}
				}

				return t;
			}


			fn calculate_normal(p: vec3f) -> vec3f
			{
				let small_step = vec3f(0.001, 0.0, 0.0);

				let gradient_x = map(p + small_step.xyy) - map(p - small_step.xyy);
				let gradient_y = map(p + small_step.yxy) - map(p - small_step.yxy);
				let gradient_z = map(p + small_step.yyx) - map(p - small_step.yyx);

				let normal = vec3f(gradient_x, gradient_y, gradient_z);

				return normalize(normal);
			}

			@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

			@vertex
			fn vs_main(in: VertexInput) -> VertexOutput {
				var out: VertexOutput;
				out.position = vec4f(in.position, 1.0);
				out.uv = in.uv;
				return out;
			}

			@fragment
			fn fs_main(in: VertexOutput) -> @location(0) vec4f {
				//let cameraPos = vec3f(0.0, 0.0, -3.0); // ray origin
				let cameraPos = uMyUniforms.cameraWorldPosition;
				let rayDirection = normalize(vec3f(in.uv, 1.0));
				let distanceTravelled = rayMarch(rayDirection, cameraPos);
				let currentPosition = cameraPos + rayDirection * distanceTravelled;
				
				var color = vec3f(0.0);
				if distanceTravelled < MAX_DIST
				{
					let lightPos = vec3f(0.0, 1.82, 0.83);
					let normal = calculate_normal(currentPosition);
					let directionToLight = normalize(currentPosition - lightPos);
					let diffuseIntensity = max(0.0, dot(normal, directionToLight));
					//color = normal * 0.5 + 0.5;
					//color = vec3f(distanceTravelled * 0.25);
					color = vec3f(1.0, 0.0, 0.0) * diffuseIntensity;
				}
				
				return vec4f(color, 1.0);
			}

			)";

			RenderSys::Shader shader("Combined", shaderSource);
			shader.type = RenderSys::ShaderType::WGSL;
			shader.stage = RenderSys::ShaderStage::VertexAndFragment;
			m_renderer->SetShader(shader);

			// Vertex buffer
			// There are 2 floats per vertex, one for x and one for y.
			// But in the end this is just a bunch of floats to the eyes of the GPU,
			// the *layout* will tell how to interpret this.
			// const std::vector<float> vertexData = {
			// 	-0.9, -0.9, 0.0, -1.0 , -1.0,
			// 	+0.9, -0.9, 0.0, 1.0 , -1.0,
			// 	+0.9, +0.9, 0.0, 1.0 , 1.0,

			// 	-0.9, -0.9, 0.0, -1.0 , -1.0,
			// 	-0.9, +0.9, 0.0, -1.0 , 1.0,
			// 	+0.9, +0.9, 0.0, 1.0 , 1.0,
			// };

			RenderSys::VertexBuffer vertexData;
			vertexData.vertices.resize(6);
			vertexData.vertices[0].position = glm::vec3(-0.9, -0.9, 0.0);
			vertexData.vertices[1].position = glm::vec3(+0.9, -0.9, 0.0);
			vertexData.vertices[2].position = glm::vec3(+0.9, +0.9, 0.0);
			vertexData.vertices[3].position = glm::vec3(-0.9, -0.9, 0.0);
			vertexData.vertices[4].position = glm::vec3(-0.9, +0.9, 0.0);
			vertexData.vertices[5].position = glm::vec3(+0.9, +0.9, 0.0);

			vertexData.vertices[0].texcoord0 = glm::vec2(-1.0, -1.0);
			vertexData.vertices[1].texcoord0 = glm::vec2(1.0, -1.0);
			vertexData.vertices[2].texcoord0 = glm::vec2(1.0, 1.0);
			vertexData.vertices[3].texcoord0 = glm::vec2(-1.0, -1.0);
			vertexData.vertices[4].texcoord0 = glm::vec2(-1.0, 1.0);
			vertexData.vertices[5].texcoord0 = glm::vec2(1.0, 1.0);

			std::vector<RenderSys::VertexAttribute> vertexAttribs(2);

			// Position attribute
			vertexAttribs[0].location = 0;
			vertexAttribs[0].format = RenderSys::VertexFormat::Float32x3;
			vertexAttribs[0].offset = 0;

			// UV attribute
			vertexAttribs[1].location = 1;
			vertexAttribs[1].format = RenderSys::VertexFormat::Float32x2;
			vertexAttribs[1].offset = offsetof(RenderSys::Vertex, texcoord0);

			RenderSys::VertexBufferLayout vertexBufferLayout;
			vertexBufferLayout.attributeCount = (uint32_t)vertexAttribs.size();
			vertexBufferLayout.attributes = vertexAttribs.data();
			vertexBufferLayout.arrayStride = sizeof(RenderSys::Vertex);
			vertexBufferLayout.stepMode = RenderSys::VertexStepMode::Vertex;


			m_renderer->SetVertexBufferData(vertexData, vertexBufferLayout);

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

			m_renderer->CreateUniformBuffer(uniformBindingLayout.binding, sizeof(MyUniforms), 1);
			m_renderer->CreateBindGroup(bindingLayoutEntries);
			m_renderer->CreatePipeline();
        }

		m_camera->OnUpdate();

		if (m_renderer)
		{
			m_renderer->BeginRenderPass();

			const float time = static_cast<float>(glfwGetTime());
			auto ssss = m_camera->GetViewMatrix();
			m_myUniformData.cameraWorldPosition = m_camera->GetPosition();
			m_myUniformData.time = time;
			m_renderer->SetUniformBufferData(0, &m_myUniformData, 0);
       		m_renderer->Render(0);
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
	MyUniforms m_myUniformData;
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