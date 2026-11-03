#pragma once
#include <RenderSys/Vulkan/VulkanContext.h>
#include <RenderSys/Vulkan/VulkanVertex.h>
#include <vector>

namespace RenderSys
{
namespace Vulkan 
{

class ShadowRenderPipeline
{

public:
    // The depth bias is dynamic state: set it with vkCmdSetDepthBias after binding the pipeline.
    ShadowRenderPipeline(VkRenderPass renderPass,
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout,
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos,
                        VkCullModeFlags cullMode);
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
                        const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageInfos,
                        VkCullModeFlags cullMode);

    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
};


} // namespace Vulkan
} // namespace RenderSys