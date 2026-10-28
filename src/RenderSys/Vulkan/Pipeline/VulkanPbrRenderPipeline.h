#pragma once
#include <RenderSys/Vulkan/VulkanContext.h>
#include <RenderSys/Vulkan/VulkanVertex.h>
#include <vector>

namespace RenderSys
{
namespace Vulkan 
{

class PbrRenderPipeline
{

public:
    PbrRenderPipeline(VkRenderPass renderPass, 
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos,
                        VkFrontFace frontFace);
    ~PbrRenderPipeline();

    PbrRenderPipeline(const PbrRenderPipeline&) = delete;
    PbrRenderPipeline& operator=(const PbrRenderPipeline&) = delete;
    PbrRenderPipeline(PbrRenderPipeline&&) = delete;
    PbrRenderPipeline& operator=(PbrRenderPipeline&&) = delete;

    VkPipeline GetPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
    void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void CreatePipeline(VkRenderPass renderPass, const Vulkan::VertexInputLayout &vertexInputLayout,
                        const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageInfos, VkFrontFace frontFace);

    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};


} // namespace Vulkan
} // namespace RenderSys