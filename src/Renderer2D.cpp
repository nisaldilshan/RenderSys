#include "Renderer2D.h"


#if (RENDERER_BACKEND == 1)
//#include <Walnut/GraphicsAPI/OpenGLGraphics.h>
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanRenderer2D.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPURenderer2D.h"
#else
#endif

Renderer2D::Renderer2D()
    : m_Width(0)
    , m_Height(0)
    , m_rendererBackend(std::make_unique<GraphicsAPI::RendererType>())
{}

void Renderer2D::OnResize(uint32_t width, uint32_t height)
{
    m_Width = width;
    m_Height = height;
    m_rendererBackend->CreateTextureToRenderInto(m_Width, m_Height);
}

Renderer2D::~Renderer2D()
{
}

void Renderer2D::Init()
{
    m_rendererBackend->Init();
    m_rendererBackend->CreateBindGroup();
}

void Renderer2D::SetShader(const char* shaderSource)
{
    m_rendererBackend->CreateShaders(shaderSource);
}

void Renderer2D::SetStandaloneShader(const char* shaderSource, uint32_t vertexShaderCallCount)
{
    m_rendererBackend->CreateStandaloneShader(shaderSource, vertexShaderCallCount);
}

void Renderer2D::SetVertexBufferData(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
{
    m_rendererBackend->CreateVertexBuffer(bufferData, bufferLength, bufferLayout);
}

void Renderer2D::SetIndexBufferData(const std::vector<uint16_t>& bufferData)
{
    m_rendererBackend->CreateIndexBuffer(bufferData);
}

void Renderer2D::CreatePipelineAndFrameBuffers()
{
    m_rendererBackend->CreatePipeline();
    m_rendererBackend->CreateFrameBuffer();
}

void Renderer2D::SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    m_rendererBackend->SetBindGroupLayoutEntry(bindGroupLayoutEntry);
}

void Renderer2D::CreateUniformBuffer(size_t bufferLength, uint32_t sizeOfUniform)
{
    m_rendererBackend->CreateUniformBuffer(bufferLength, sizeOfUniform);
}

void Renderer2D::SetUniformBufferData(const void* bufferData, uint32_t uniformIndex)
{
    m_rendererBackend->SetUniformData(bufferData, uniformIndex);
}

void* Renderer2D::GetDescriptorSet()
{
    return m_rendererBackend->GetDescriptorSet();
}

void Renderer2D::SimpleRender()
{
    m_rendererBackend->SimpleRender();
}

void Renderer2D::Render()
{
    m_rendererBackend->Render();
}

void Renderer2D::RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount)
{
    m_rendererBackend->RenderIndexed(uniformIndex, dynamicOffsetCount);
}

void Renderer2D::BeginRenderPass()
{
    m_rendererBackend->BeginRenderPass();
}

void Renderer2D::EndRenderPass()
{
    m_rendererBackend->EndRenderPass();
}
