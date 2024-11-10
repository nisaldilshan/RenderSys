#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include <imgui_impl_glfw.h>
#include <Walnut/ImageFormat.h>

#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include "Buffer.h"


#include "RenderUtil.h"
#include "Shader.h"

namespace GraphicsAPI
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
#endif
}

namespace RenderSys
{
    
class Renderer3D
{
public:
    Renderer3D();
    ~Renderer3D();
    
    Renderer3D(const Renderer3D&) = delete;
	Renderer3D &operator=(const Renderer3D&) = delete;
	Renderer3D(Renderer3D&&) = delete;
	Renderer3D &operator=(Renderer3D&&) = delete;

    void Init();
    void OnResize(uint32_t width, uint32_t height);
    void SetShader(RenderSys::Shader& shader);
    void SetStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount);
    void SetVertexBufferData(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout);
    void SetIndexBufferData(const std::vector<uint16_t>& bufferData);
    void CreatePipeline();
    void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
    void CreateTexture(uint32_t width, uint32_t height, const void* textureData, uint32_t mipMapLevelCount);
    void CreateTextureSampler();
    void SetClearColor(glm::vec4 clearColor);
    void CreateUniformBuffer(size_t bufferLength, UniformBuf::UniformType type, uint32_t sizeOfUniform, uint32_t bindingIndex);
    void SetUniformBufferData(UniformBuf::UniformType type, const void* bufferData, uint32_t uniformIndex);
    uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
    void SimpleRender();
    void Render(uint32_t uniformIndex);
    void RenderIndexed(uint32_t uniformIndex);
    void BeginRenderPass();
    void EndRenderPass();

    void* GetDescriptorSet() const;
private:
    uint32_t m_Width = 0, m_Height = 0;
    std::unique_ptr<GraphicsAPI::RendererType> m_rendererBackend;
};

} // namespace RenderSys



