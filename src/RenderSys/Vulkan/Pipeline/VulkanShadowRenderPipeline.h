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
    void CreatePipeline(VkRenderPass renderPass, const Vulkan::VertexInputLayout &vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageInfos);

    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
};


} // namespace Vulkan
} // namespace RenderSys