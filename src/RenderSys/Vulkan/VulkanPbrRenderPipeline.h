#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/Vulkan/VulkanVertex.h>

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
                        const std::vector<VkPipelineShaderStageCreateInfo>& m_shaderStageInfos);
    ~PbrRenderPipeline();

    PbrRenderPipeline(const PbrRenderPipeline&) = delete;
    PbrRenderPipeline& operator=(const PbrRenderPipeline&) = delete;
    PbrRenderPipeline(PbrRenderPipeline&&) = delete;
    PbrRenderPipeline& operator=(PbrRenderPipeline&&) = delete;

    VkPipeline GetPipeline() const { return m_Pipeline; }

private:
    void CreatePipelineLayout(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
    void CreatePipeline(VkRenderPass renderPass);

    VkPipelineLayout m_PipelineLayout;
    VkPipeline m_Pipeline;
};


} // namespace Vulkan
} // namespace RenderSys