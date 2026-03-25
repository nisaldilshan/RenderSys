#pragma once
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>
#include <RenderSys/Vulkan/VulkanVertex.h>

namespace RenderSys
{
namespace Vulkan 
{

class Render2DPipeline
{

public:
    Render2DPipeline(VkRenderPass renderPass, 
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos);
    ~Render2DPipeline();

    Render2DPipeline(const Render2DPipeline&) = delete;
    Render2DPipeline& operator=(const Render2DPipeline&) = delete;
    Render2DPipeline(Render2DPipeline&&) = delete;
    Render2DPipeline& operator=(Render2DPipeline&&) = delete;

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