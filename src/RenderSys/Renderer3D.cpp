#include "Renderer3D.h"

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanRenderer3D.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPURenderer3D.h"
#else
static_assert(false);
#endif

using namespace RenderSys;

Renderer3D::Renderer3D()
    : m_Width(0)
    , m_Height(0)
    , m_rendererBackend(std::make_unique<RenderSys::RendererType>())
{}

Renderer3D::~Renderer3D()
{}

void Renderer3D::OnResize(uint32_t width, uint32_t height)
{
    m_rendererBackend->DestroyImages();

    m_Width = width;
    m_Height = height;
    m_rendererBackend->CreateImageToRender(m_Width, m_Height);
    m_rendererBackend->CreateDepthImage();
    m_rendererBackend->CreateFrameBuffer();
}

void Renderer3D::Init()
{
    m_rendererBackend->Init();
    OnResize(1, 1);
}

void Renderer3D::SetShader(RenderSys::Shader& shader)
{
    m_rendererBackend->CreateShader(shader);
}

uint32_t Renderer3D::SetVertexBufferData(const VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout)
{
    return m_rendererBackend->CreateVertexBuffer(bufferData, bufferLayout);
}

void Renderer3D::SetIndexBufferData(uint32_t vertexBufferID, const std::vector<uint32_t>& bufferData)
{
    m_rendererBackend->CreateIndexBuffer(vertexBufferID, bufferData);
}

void Renderer3D::CreatePipeline()
{
    m_rendererBackend->CreatePipeline();
}

void Renderer3D::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    m_rendererBackend->CreateBindGroup(bindGroupLayoutEntries);
}

void Renderer3D::CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture)
{
    m_rendererBackend->CreateTexture(binding, texture);
}

void Renderer3D::SetClearColor(glm::vec4 clearColor)
{
    m_rendererBackend->SetClearColor(clearColor);
}

void Renderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfUniform, size_t bufferLength)
{
    m_rendererBackend->CreateUniformBuffer(binding, sizeOfUniform);
}

void Renderer3D::SetUniformBufferData(uint32_t binding, const void* bufferData, uint32_t uniformIndex)
{
    m_rendererBackend->SetUniformData(binding, bufferData);
}

void Renderer3D::BindResources()
{
    m_rendererBackend->BindResources();
}

void* Renderer3D::GetDescriptorSet() const
{
    return m_rendererBackend->GetDescriptorSet();
}

void Renderer3D::Destroy()
{
    return m_rendererBackend->Destroy();
}

void Renderer3D::Render(uint32_t uniformIndex)
{
    m_rendererBackend->Render();
}

void Renderer3D::RenderIndexed(uint32_t uniformIndex)
{
    m_rendererBackend->RenderIndexed();
}

void Renderer3D::BeginFrame()
{
    m_rendererBackend->ResetCommandBuffer();
}

void Renderer3D::EndFrame()
{
    m_rendererBackend->SubmitCommandBuffer();
}

void Renderer3D::RenderMesh(const RenderSys::Mesh& mesh)
{
    m_rendererBackend->RenderMesh(mesh);
}

void Renderer3D::BeginRenderPass()
{
    m_rendererBackend->BeginRenderPass();
}

void Renderer3D::EndRenderPass()
{
    m_rendererBackend->EndRenderPass();
}

void Renderer3D::CreateShadowMap(uint32_t mapWidth, uint32_t mapHeight)
{
    m_rendererBackend->CreateShadowMap(mapWidth, mapHeight);
}

void Renderer3D::CreateShadowPipeline()
{
    m_rendererBackend->CreateShadowPipeline();
}

void Renderer3D::ShadowPass(entt::registry &entityRegistry)
{
    m_rendererBackend->BeginShadowMapPass();
    m_rendererBackend->RenderShadowMap(entityRegistry);
    m_rendererBackend->EndShadowMapPass();
}

void Renderer3D::OnDebugView()
{
    m_rendererBackend->OnDebugView();
}

std::vector<uint8_t>& Renderer3D::GetRenderedImageData()
{
    return m_rendererBackend->GetRenderedImageDataToCPUSide();
}
