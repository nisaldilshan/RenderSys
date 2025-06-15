#include "VulkanShadowRenderPipeline.h"

namespace RenderSys::Vulkan 
{
ShadowRenderPipeline::ShadowRenderPipeline(VkRenderPass renderPass, 
                        std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
                        const Vulkan::VertexInputLayout& vertexInputLayout, 
                        const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos)
{

}

ShadowRenderPipeline::~ShadowRenderPipeline()
{
    
}

} // namespace RenderSys::Vulkan 
