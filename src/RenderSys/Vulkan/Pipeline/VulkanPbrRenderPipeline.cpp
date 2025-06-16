#include "VulkanPbrRenderPipeline.h"

#include <RenderSys/Material.h>
#include <RenderSys/Vulkan/Pipeline/VulkanPipeline.h>

#include <vector>
#include <iostream>

namespace RenderSys {

namespace Vulkan {

PbrRenderPipeline::PbrRenderPipeline(VkRenderPass renderPass,
    std::vector<VkDescriptorSetLayout> &descriptorSetLayouts,
    const Vulkan::VertexInputLayout& vertexInputLayout, 
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStageInfos) 
{
    CreatePipelineLayout(descriptorSetLayouts);
    CreatePipeline(renderPass, vertexInputLayout, shaderStageInfos);
}

PbrRenderPipeline::~PbrRenderPipeline() 
{
    if (m_Pipeline)
    {
        vkDestroyPipeline(GraphicsAPI::Vulkan::GetDevice(), m_Pipeline, nullptr);
        m_Pipeline = VK_NULL_HANDLE;
    }

    if (m_PipelineLayout)
    {
        vkDestroyPipelineLayout(GraphicsAPI::Vulkan::GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
}

void PbrRenderPipeline::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts)
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

void PbrRenderPipeline::CreatePipeline(VkRenderPass renderPass, const Vulkan::VertexInputLayout &vertexInputLayout,
                                       const std::vector<VkPipelineShaderStageCreateInfo> &shaderStageInfos)
{
    assert(m_PipelineLayout != VK_NULL_HANDLE);

    std::cout << "Creating render pipeline..." << std::endl;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    std::vector<VkVertexInputBindingDescription> vertexBindingDescs;
    std::vector<VkVertexInputAttributeDescription> vertexAttribDescs;
    assert(vertexInputLayout.m_vertexAttribDescs.size() > 0);
    vertexBindingDescs.push_back(vertexInputLayout.m_vertexBindingDescs);
    for (const auto &vertextAttribDesc : vertexInputLayout.m_vertexAttribDescs)
    {
        vertexAttribDescs.push_back(vertextAttribDesc);
    }

    vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescs.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexAttribDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttribDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = nullptr;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = nullptr;

    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Use the new alpha directly
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Discard the old alpha
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendingInfo = Pipeline::CreateColorBlendState(colorBlendAttachment);

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = VK_TRUE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = VK_FALSE;

    std::vector<VkDynamicState> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynStatesInfo{};
    dynStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynStatesInfo.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynStatesInfo.pDynamicStates = dynStates.data();

    assert(shaderStageInfos.size() > 0);
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = shaderStageInfos.size();
    pipelineCreateInfo.pStages = shaderStageInfos.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizerInfo = Vulkan::Pipeline::getRasterizerInfo();
    pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
    pipelineCreateInfo.pDynamicState = &dynStatesInfo;
    pipelineCreateInfo.layout = m_PipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(GraphicsAPI::Vulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
        std::cout << "error: could not create rendering pipeline" << std::endl;
    }

    // can save memory by calling DestroyShaders() after pipeline have been created
    // currently not possible, as pipeline get recreated every window get resized
    assert(m_Pipeline != VK_NULL_HANDLE);
    std::cout << "Render pipeline: " << m_Pipeline << std::endl;
}

} // namespace Vulkan

} // namespace RenderSys
