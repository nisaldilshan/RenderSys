#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include "Buffer.h"
#include "RenderUtil.h"
#include "Shader.h"
#include <RenderSys/Texture.h>
#include <RenderSys/Scene/Mesh.h>
#include <RenderSys/Components/MeshComponent.h>
#include <entt/entt.hpp>

namespace RenderSys
{
#if (RENDERSYS_BACKEND == 1)
class OpenGLRenderer3D;
typedef OpenGLRenderer3D RendererType;
#elif (RENDERSYS_BACKEND == 2)
class VulkanRenderer3D;
typedef VulkanRenderer3D RendererType;
#elif (RENDERSYS_BACKEND == 3)
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
    void CreatePipeline(Handedness cameraHandedness = Handedness::Left);
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
    void RenderMesh(const RenderSys::MeshComponent& meshComponent);
    void BeginRenderPass();
    void EndRenderPass();

    // A depth map at binding 2 of the main bind group, read as sampler2DShadow (see Texture::createShadowMapTexture)
    void CreateShadowMap(uint32_t mapWidth, uint32_t mapHeight);
    // Builds the shadow pass's pipeline, or rebuilds it when called again. Front culls the faces that face the light.
    void CreateShadowPipeline(CullMode cullMode = CullMode::None);
    // Rasterizer depth bias for the shadow pass, in the units of vkCmdSetDepthBias: constantFactor in the depth
    // format's smallest steps, slopeFactor times the depth slope of each triangle
    void SetShadowDepthBias(float constantFactor, float slopeFactor);
    void ShadowPass(entt::registry& entityRegistry);
    void* GetDescriptorSet() const;
    void Destroy();
    // The shadow map as an ImGui texture, its depth as greyscale; 0 without a shadow map or a UI, or before the
    // first ShadowPass
    uint64_t GetShadowMapImGuiTexture() const;
    std::vector<uint8_t>& GetRenderedImageData();

private:
    uint32_t m_Width = 0, m_Height = 0;
    std::unique_ptr<RendererType> m_rendererBackend;
};

} // namespace RenderSys



