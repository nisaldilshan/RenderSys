#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Vulkan/VulkanVertex.h>

namespace RenderSys
{
namespace Vulkan 
{

class ShadowRenderPipeline
{

public:
    ShadowRenderPipeline(VkRenderPass renderPass, 
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos);
    ~ShadowRenderPipeline();

    ShadowRenderPipeline(const ShadowRenderPipeline&) = delete;
    ShadowRenderPipeline& operator=(const ShadowRenderPipeline&) = delete;
    ShadowRenderPipeline(ShadowRenderPipeline&&) = delete;
    ShadowRenderPipeline& operator=(ShadowRenderPipeline&&) = delete;

    VkPipeline GetPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
    void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void CreatePipeline(VkRenderPass renderPass, const Vulkan::VertexInputLayout &vertexInputLayout);

    std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};


} // namespace Vulkan
} // namespace RenderSys