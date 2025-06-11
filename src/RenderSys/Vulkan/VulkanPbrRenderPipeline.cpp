#include "VulkanPbrRenderPipeline.h"

#include <RenderSys/Material.h>

#include <vector>

namespace RenderSys {

namespace Vulkan {

PbrRenderPipeline::PbrRenderPipeline(VkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) 
{
    CreatePipelineLayout(descriptorSetLayouts);
    CreatePipeline(renderPass);
}

PbrRenderPipeline::~PbrRenderPipeline() 
{
    vkDestroyPipelineLayout(GraphicsAPI::Vulkan::GetDevice(), m_PipelineLayout, nullptr);
}

void PbrRenderPipeline::CreatePipelineLayout(std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) 
{
    // VkPushConstantRange pushConstantRange0{};
    // pushConstantRange0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // pushConstantRange0.offset = 0;
    // pushConstantRange0.size = sizeof(VertexCtrl);

    // VkPushConstantRange pushConstantRange1{};
    // pushConstantRange1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    // pushConstantRange1.offset = sizeof(VertexCtrl);
    // pushConstantRange1.size = sizeof(PbrMaterial::PbrMaterialProperties);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(RenderSys::MaterialProperties);

    std::array<VkPushConstantRange, 1> pushConstantRanges = {pushConstantRange}; //{pushConstantRange0, pushConstantRange1};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
    auto result = vkCreatePipelineLayout(GraphicsAPI::Vulkan::GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
    if (result != VK_SUCCESS)
    {
        GraphicsAPI::Vulkan::check_vk_result(result);
    }
}

void PbrRenderPipeline::CreatePipeline(VkRenderPass renderPass) {
}

}

}
