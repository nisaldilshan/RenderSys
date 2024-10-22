#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <imgui_impl_glfw.h>
#include <Walnut/ImageFormat.h>


#include "RenderUtil.h"
#include "Shader.h"

namespace GraphicsAPI
{
#if (RENDERER_BACKEND == 1)
class OpenGLRenderer2D;
typedef OpenGLRenderer2D RendererType;
#elif (RENDERER_BACKEND == 2)
class VulkanRenderer2D;
typedef VulkanRenderer2D RendererType;
#elif (RENDERER_BACKEND == 3)
class WebGPURenderer2D;
typedef WebGPURenderer2D RendererType;
#else
static_assert(false);
#endif
}

namespace RenderSys
{
class Renderer2D
{
public:
    Renderer2D();
    ~Renderer2D();

    Renderer2D(const Renderer2D&) = delete;
	Renderer2D &operator=(const Renderer2D&) = delete;
	Renderer2D(Renderer2D&&) = delete;
	Renderer2D &operator=(Renderer2D&&) = delete;

    void Init();
    void OnResize(uint32_t width, uint32_t height);
    void SetShader(RenderSys::Shader& shader);
    void SetStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
    void SetVertexBufferData(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
    void SetIndexBufferData(const std::vector<uint16_t>& bufferData);
    void CreatePipeline();
    void CreateBindGroup(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntries);
    void CreateUniformBuffer(size_t bufferLength, uint32_t sizeOfUniform);
    
    void SetUniformBufferData(const void* bufferData, uint32_t uniformIndex);
    uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
    void SimpleRender();
    void Render();
    void RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount);
    void BeginRenderPass();
    void EndRenderPass();

    void* GetDescriptorSet() const;
    void Destroy();
private:
    uint32_t m_Width = 0, m_Height = 0;
    std::unique_ptr<GraphicsAPI::RendererType> m_rendererBackend;
};

} // namespace RenderSys


