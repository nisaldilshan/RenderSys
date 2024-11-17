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
    , m_rendererBackend(std::make_unique<GraphicsAPI::RendererType>())
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
}

void Renderer3D::SetShader(RenderSys::Shader& shader)
{
    m_rendererBackend->CreateShaders(shader);
}

void Renderer3D::SetStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount)
{
    m_rendererBackend->CreateStandaloneShader(shader, vertexShaderCallCount);
}

void Renderer3D::SetVertexBufferData(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
{
    m_rendererBackend->CreateVertexBuffer(bufferData, bufferLength, bufferLayout);
}

void Renderer3D::SetIndexBufferData(const std::vector<uint16_t>& bufferData)
{
    m_rendererBackend->CreateIndexBuffer(bufferData);
}

void Renderer3D::CreatePipeline()
{
    m_rendererBackend->CreatePipeline();
}

void Renderer3D::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    m_rendererBackend->CreateBindGroup(bindGroupLayoutEntries);
}

void Renderer3D::CreateTexture(uint32_t width, uint32_t height, const void* textureData, uint32_t mipMapLevelCount)
{
    m_rendererBackend->CreateTexture(width, height, textureData, mipMapLevelCount);
}

void Renderer3D::CreateTextureSampler()
{
    m_rendererBackend->CreateTextureSampler();
}

void Renderer3D::SetClearColor(glm::vec4 clearColor)
{
    m_rendererBackend->SetClearColor(clearColor);
}

void Renderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfUniform, size_t bufferLength)
{
    m_rendererBackend->CreateUniformBuffer(binding, sizeOfUniform, bufferLength);
}

void Renderer3D::SetUniformBufferData(uint32_t binding, const void* bufferData, uint32_t uniformIndex)
{
    m_rendererBackend->SetUniformData(binding, bufferData, uniformIndex);
}

void RenderSys::Renderer3D::BindResources()
{
    m_rendererBackend->BindResources();
}

void* Renderer3D::GetDescriptorSet() const
{
    return m_rendererBackend->GetDescriptorSet();
}

void RenderSys::Renderer3D::Destroy()
{
    return m_rendererBackend->Destroy();
}

void Renderer3D::Render(uint32_t uniformIndex)
{
    m_rendererBackend->Render(uniformIndex);
}

void Renderer3D::RenderIndexed(uint32_t uniformIndex)
{
    m_rendererBackend->RenderIndexed(uniformIndex);
}

void Renderer3D::BeginRenderPass()
{
    m_rendererBackend->BeginRenderPass();
}

void Renderer3D::EndRenderPass()
{
    m_rendererBackend->EndRenderPass();
}
