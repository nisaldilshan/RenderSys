#pragma once
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

namespace RenderSys
{
namespace Vulkan 
{

class PbrRenderPipeline
{

public:
    PbrRenderPipeline(VkRenderPass renderPass, std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
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