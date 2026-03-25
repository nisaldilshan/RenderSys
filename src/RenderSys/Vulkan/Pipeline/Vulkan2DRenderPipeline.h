#pragma once
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>
#include <RenderSys/Vulkan/VulkanVertex.h>

namespace RenderSys
{
namespace Vulkan 
{

class 2DRenderPipeline
{

public:
    2DRenderPipeline(VkRenderPass renderPass, 
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos);
    ~2DRenderPipeline();

    2DRenderPipeline(const 2DRenderPipeline&) = delete;
    2DRenderPipeline& operator=(const 2DRenderPipeline&) = delete;
    2DRenderPipeline(2DRenderPipeline&&) = delete;
    2DRenderPipeline& operator=(2DRenderPipeline&&) = delete;

    VkPipeline GetPipeline() const { return m_Pipeline; }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
    void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void CreatePipeline(VkRenderPass renderPass, const Vulkan::VertexInputLayout &vertexInputLayout,
                        const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageInfos);

    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};


} // namespace Vulkan
} // namespace RenderSys