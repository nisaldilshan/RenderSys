#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include "Buffer.h"
#include "RenderUtil.h"
#include "Shader.h"
#include "Texture.h"
#include <RenderSys/Scene/Mesh.h>

namespace RenderSys
{
#if (RENDERER_BACKEND == 1)
class OpenGLRenderer3D;
typedef OpenGLRenderer3D RendererType;
#elif (RENDERER_BACKEND == 2)
class VulkanRenderer3D;
typedef VulkanRenderer3D RendererType;
#elif (RENDERER_BACKEND == 3)
class WebGPURenderer3D;
typedef WebGPURenderer3D RendererType;
#else
static_assert(false);
#endif

class Renderer3D
{
public:
    Renderer3D();
    ~Renderer3D();
    
    Renderer3D(const Renderer3D&) = delete;
	Renderer3D &operator=(const Renderer3D&) = delete;
	Renderer3D(Renderer3D&&) = delete;
	Renderer3D &operator=(Renderer3D&&) = delete;

    uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
    void Init();
    void OnResize(uint32_t width, uint32_t height);
    void SetShader(RenderSys::Shader& shader);
    uint32_t SetVertexBufferData(const VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout);
    void SetIndexBufferData(uint32_t vertexBufferID, const std::vector<uint32_t>& bufferData);
    void CreatePipeline();
    void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture);
    void SetClearColor(glm::vec4 clearColor);
    void CreateUniformBuffer(uint32_t binding, uint32_t sizeOfUniform, size_t bufferLength);
    void SetUniformBufferData(uint32_t binding, const void* bufferData, uint32_t uniformIndex);
    void BindResources();
    void Render(uint32_t uniformIndex);
    void RenderIndexed(uint32_t uniformIndex);
    void BeginFrame();
    void EndFrame();
    void RenderMesh(const RenderSys::Mesh& mesh);
    void BeginRenderPass();
    void EndRenderPass();
    void ShadowPass();
    void* GetDescriptorSet() const;
    void Destroy();
private:
    uint32_t m_Width = 0, m_Height = 0;
    std::unique_ptr<RendererType> m_rendererBackend;
};

} // namespace RenderSys



