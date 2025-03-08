#include "VulkanRenderer3D.h"
#include "VulkanRendererUtils.h"

#include <array>
#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace GraphicsAPI
{

bool VulkanRenderer3D::Init()
{
    if (!m_vma)
    {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = Vulkan::GetPhysicalDevice();
        allocatorInfo.device = Vulkan::GetDevice();
        allocatorInfo.instance = Vulkan::GetInstance();
        if (vmaCreateAllocator(&allocatorInfo, &m_vma) != VK_SUCCESS) {
            std::cout << "error: could not init VMA" << std::endl;
            return false;
        }
    }

    CreateRenderPass();
    CreateCommandBuffers();


    return true;
}

void VulkanRenderer3D::CreateImageToRender(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    VkImageCreateInfo renderImageInfo{};
    renderImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    renderImageInfo.imageType = VK_IMAGE_TYPE_2D;
    renderImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    renderImageInfo.extent = VkExtent3D{m_width, m_height, 1};
    renderImageInfo.mipLevels = 1;
    renderImageInfo.arrayLayers = 1;
    renderImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    renderImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    renderImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    renderImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    renderImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vmaCreateImage(m_vma, &renderImageInfo, &allocInfo, &m_ImageToRenderInto, &m_renderImageMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }

    m_imageViewToRenderInto = CreateImageView(m_ImageToRenderInto, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    CreateDefaultTextureSampler();
    m_descriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_defaultTextureSampler, m_imageViewToRenderInto, VK_IMAGE_LAYOUT_GENERAL);
}

void VulkanRenderer3D::CreateDepthImage()
{
    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
    depthImageInfo.extent = VkExtent3D{m_width, m_height, 1};
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vmaCreateImage(m_vma, &depthImageInfo, &allocInfo, &m_depthimage, &m_depthimageMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }

    m_depthimageView = CreateImageView(m_depthimage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer3D::CreateShaders(RenderSys::Shader& shader)
{
    assert(shader.type == RenderSys::ShaderType::SPIRV);
    std::vector<uint32_t> compiledShader;
    auto shaderMapIter = m_shaderMap.find(shader.GetName());
    if (shaderMapIter == m_shaderMap.end())
    {
        compiledShader = RenderSys::ShaderUtils::compile_file(shader.GetName(), shader);
        assert(compiledShader.size() > 0);
        m_shaderMap.emplace(shader.GetName(), compiledShader);
    }
    else
    {
        compiledShader = shaderMapIter->second;
    }

    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = sizeof(uint32_t) * compiledShader.size();
    shaderCreateInfo.pCode = compiledShader.data();

    VkShaderModule shaderModule = 0;
    if (vkCreateShaderModule(Vulkan::GetDevice(), &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "could not load vertex shader" << std::endl;
        return;
    }

    if (shaderModule == VK_NULL_HANDLE) {
        std::cout << "error: could not load shaders" << std::endl;
        return;
    }

    std::cout << "Created Shader module, Name:" << shader.GetName() << ", Ptr:" << shaderModule << std::endl;

    VkShaderStageFlagBits shaderStageBits;
    if (shader.stage == RenderSys::ShaderStage::Vertex)
    {
        shaderStageBits = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (shader.stage == RenderSys::ShaderStage::Fragment)
    {
        shaderStageBits = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else
    {
        assert(false);
    }
    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = shaderStageBits;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main"; // entrypoint

    m_shaderStageInfos.push_back(stageInfo);
}

void VulkanRenderer3D::DestroyImages()
{
    if (m_descriptorSet)
    {
        vkQueueWaitIdle(Vulkan::GetDeviceQueue());
        ImGui_ImplVulkan_RemoveTexture(m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }

    if (m_frameBuffer)
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());
        vkDestroyFramebuffer(Vulkan::GetDevice(), m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }
    
    if (m_imageViewToRenderInto != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Vulkan::GetDevice(), m_imageViewToRenderInto, nullptr);
        m_imageViewToRenderInto = VK_NULL_HANDLE;
    }
    if (m_ImageToRenderInto != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_vma, m_ImageToRenderInto, m_renderImageMemory);
        m_ImageToRenderInto = VK_NULL_HANDLE;
    }

    if (m_depthimageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Vulkan::GetDevice(), m_depthimageView, nullptr);
        m_depthimageView = VK_NULL_HANDLE;
    }
    if (m_depthimage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_vma, m_depthimage, m_depthimageMemory);
        m_depthimage = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::DestroyPipeline()
{
    if (m_pipeline)
    {
        vkDestroyPipeline(Vulkan::GetDevice(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout)
    {
        vkDestroyPipelineLayout(Vulkan::GetDevice(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::DestroyBindGroup()
{
    if (m_mainBindGroup && m_bindGroupPool)
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());
        // when you destroy a descriptor pool, all descriptor sets allocated from that pool are automatically destroyed
        vkDestroyDescriptorPool(Vulkan::GetDevice(), m_bindGroupPool, nullptr);
        m_mainBindGroup = VK_NULL_HANDLE;
        m_bindGroupPool = VK_NULL_HANDLE;
    }

    if (m_bindGroupLayout)
    {
        vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), m_bindGroupLayout, nullptr);
        m_bindGroupLayout = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::DestroyBuffers()
{
    for (auto [id, vertexIndexBufferInfo] : m_vertexIndexBufferInfoMap)
    {
        if (vertexIndexBufferInfo.m_vertexBuffer != VK_NULL_HANDLE && vertexIndexBufferInfo.m_vertexBufferMemory != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_vma, vertexIndexBufferInfo.m_vertexBuffer, vertexIndexBufferInfo.m_vertexBufferMemory);
            vertexIndexBufferInfo.m_vertexBuffer = VK_NULL_HANDLE;
            vertexIndexBufferInfo.m_vertexBufferMemory = VK_NULL_HANDLE;
        }
    
        if (vertexIndexBufferInfo.m_indexBuffer != VK_NULL_HANDLE && vertexIndexBufferInfo.m_indexBufferMemory != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(m_vma, vertexIndexBufferInfo.m_indexBuffer, vertexIndexBufferInfo.m_indexBufferMemory);
            vertexIndexBufferInfo.m_indexBuffer = VK_NULL_HANDLE;
            vertexIndexBufferInfo.m_indexBufferMemory = VK_NULL_HANDLE;
        }
    }
    m_vertexIndexBufferInfoMap.clear();

    for (auto& [_ , uniformBufferTuple] : m_uniformBuffers)
    {
        VkDescriptorBufferInfo& bufferInfo = std::get<0>(uniformBufferTuple);
        VmaAllocation& uniformBufferMemory = std::get<1>(uniformBufferTuple);
        vmaDestroyBuffer(m_vma, bufferInfo.buffer, uniformBufferMemory);
    }
    m_uniformBuffers.clear();

    if (m_commandBuffer && m_commandPool)
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());
        // when you destroy a command pool, all command buffers allocated from that pool are automatically destroyed
        vkDestroyCommandPool(Vulkan::GetDevice(), m_commandPool, nullptr);
        m_commandBuffer = VK_NULL_HANDLE;
        m_commandPool = VK_NULL_HANDLE;
    }    
}

void VulkanRenderer3D::DestroyShaders()
{
    for (auto& shaderStageInfo : m_shaderStageInfos)
    {
        vkDestroyShaderModule(Vulkan::GetDevice(), shaderStageInfo.module, nullptr);
    }

    m_shaderStageInfos.clear();
}

void VulkanRenderer3D::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    assert(bindGroupLayoutEntries.size() >= 1);
    assert(!m_bindGroupLayout && !m_bindGroupPool && !m_mainBindGroup);

    std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCountMap;
    std::vector<VkDescriptorSetLayoutBinding> mainBindGroupBindings;
    for (const auto &bindGroupLayoutEntry : bindGroupLayoutEntries)
    {
        auto vkBinding = GetVulkanBindGroupLayoutEntry(bindGroupLayoutEntry);
        mainBindGroupBindings.push_back(vkBinding);

        auto mapIter = descriptorTypeCountMap.find(vkBinding.descriptorType);
        if (mapIter != descriptorTypeCountMap.end())
        {
            mapIter->second++;
        }
        else
        {
            descriptorTypeCountMap.insert({vkBinding.descriptorType, 1});
        }
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = mainBindGroupBindings.size();
    layoutInfo.pBindings = mainBindGroupBindings.data();

    if (vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &layoutInfo, nullptr, &m_bindGroupLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(descriptorTypeCountMap.size());
    for (const auto& [type, size] : descriptorTypeCountMap) {
        poolSizes.emplace_back(VkDescriptorPoolSize{type, size}); 
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_bindGroupPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_bindGroupPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_bindGroupLayout;

    if (vkAllocateDescriptorSets(Vulkan::GetDevice(), &allocInfo, &m_mainBindGroup) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (auto& bindGroupBinding : mainBindGroupBindings)
    {
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_mainBindGroup;
        descriptorWrite.dstBinding = bindGroupBinding.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = bindGroupBinding.descriptorType;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = nullptr;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        const auto& uniformBufferIter = m_uniformBuffers.find(bindGroupBinding.binding);
        if (uniformBufferIter != m_uniformBuffers.end())
        {
            VkDescriptorBufferInfo& bufferInfo = std::get<0>(uniformBufferIter->second);
            descriptorWrite.pBufferInfo = &bufferInfo;
        }
        
        const auto& textureIter = m_textures.find(bindGroupBinding.binding);
        if (textureIter != m_textures.end())
        {
            VkDescriptorImageInfo& imageInfo = std::get<2>(textureIter->second);
            imageInfo.sampler = m_defaultTextureSampler;
            descriptorWrite.pImageInfo = &imageInfo;
        }

        if (descriptorWrite.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            assert(false);
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = m_defaultTextureSampler;
            descriptorWrite.pImageInfo = &imageInfo;
        }
        
        vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
    }
}

void VulkanRenderer3D::CreatePipelineLayout()
{
    if (!m_pipelineLayout)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::vector<VkDescriptorSetLayout> layouts{m_bindGroupLayout, m_materialBindGroupLayout};
        if (!m_bindGroupLayout)
        {
            pipelineLayoutCreateInfo.setLayoutCount = 0;
            pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        }
        else if (!m_materialBindGroupLayout)
        {
            pipelineLayoutCreateInfo.setLayoutCount = 1;
            pipelineLayoutCreateInfo.pSetLayouts = &m_bindGroupLayout;
        }
        else
        {
            pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
            pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
        }
        
        VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.size = sizeof(uint32_t);
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(Vulkan::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            std::cout << "error: could not create pipeline layout" << std::endl;
        }        
    }
}

void VulkanRenderer3D::CreateRenderPass()
{
    VkAttachmentDescription colorAtt{};
    colorAtt.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentReference colorAttRef{};
    colorAttRef.attachment = 0;
    colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAtt{};
    depthAtt.flags = 0;
    depthAtt.format = VK_FORMAT_D32_SFLOAT;
    depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttRef{};
    depthAttRef.attachment = 1;
    depthAttRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc{};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttRef;
    subpassDesc.pDepthStencilAttachment = &depthAttRef;

    VkSubpassDependency subpassDep{};
    subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDep.dstSubpass = 0;
    subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.srcAccessMask = 0;
    subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDep{};
    depthDep.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDep.dstSubpass = 0;
    depthDep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDep.srcAccessMask = 0;
    depthDep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[] = {subpassDep, depthDep};
    VkAttachmentDescription attachments[] = {colorAtt, depthAtt};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(Vulkan::GetDevice(), &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
    {
        std::cout << "error; could not create renderpass" << std::endl;
        assert(false);
    }
}

void VulkanRenderer3D::CreateCommandBuffers()
{
    auto queueFamilyIndices = Vulkan::FindQueueFamilies();
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    auto err = vkCreateCommandPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_commandPool);
    Vulkan::check_vk_result(err);

    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = m_commandPool;
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;
    err = vkAllocateCommandBuffers(Vulkan::GetDevice(), &cmdBufAllocateInfo, &m_commandBuffer);
    Vulkan::check_vk_result(err);
}

void VulkanRenderer3D::DestroyRenderPass()
{
    vkDeviceWaitIdle(Vulkan::GetDevice());
    vkDestroyRenderPass(Vulkan::GetDevice(), m_renderpass, nullptr);
}

void VulkanRenderer3D::CreatePipeline()
{
    if (m_pipeline)
        return;

    std::cout << "Creating render pipeline..." << std::endl;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    assert(m_vertexIndexBufferInfoMap.size() == 1);
    vertexInputInfo.vertexBindingDescriptionCount = m_vertexIndexBufferInfoMap.size();
    vertexInputInfo.pVertexBindingDescriptions = &m_vertexIndexBufferInfoMap[1].m_vertextBindingDescs;
    vertexInputInfo.vertexAttributeDescriptionCount = m_vertexIndexBufferInfoMap[1].m_vertextAttribDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = m_vertexIndexBufferInfoMap[1].m_vertextAttribDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = {m_width, m_height};

    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.lineWidth = 1.0f;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
    colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingInfo.logicOpEnable = VK_FALSE;
    colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingInfo.attachmentCount = 1;
    colorBlendingInfo.pAttachments = &colorBlendAttachment;
    colorBlendingInfo.blendConstants[0] = 0.0f;
    colorBlendingInfo.blendConstants[1] = 0.0f;
    colorBlendingInfo.blendConstants[2] = 0.0f;
    colorBlendingInfo.blendConstants[3] = 0.0f;

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

    assert(m_shaderStageInfos.size() > 0);
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = m_shaderStageInfos.size();
    pipelineCreateInfo.pStages = m_shaderStageInfos.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
    pipelineCreateInfo.pDynamicState = &dynStatesInfo;

    CreatePipelineLayout();
    pipelineCreateInfo.layout = m_pipelineLayout;
    pipelineCreateInfo.renderPass = m_renderpass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(Vulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS) {
        std::cout << "error: could not create rendering pipeline" << std::endl;
    }

    // can save memory by calling DestroyShaders() after pipeline have been created
    // currently not possible, as pipeline get recreated every window get resized
    assert(m_pipeline != VK_NULL_HANDLE);
    std::cout << "Render pipeline: " << m_pipeline << std::endl;
}

void VulkanRenderer3D::CreateFrameBuffer()
{
    assert(m_imageViewToRenderInto);
    assert(m_depthimageView);
    VkImageView frameBufferAttachments[] = { m_imageViewToRenderInto, m_depthimageView };
    VkFramebufferCreateInfo FboInfo{};
    FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FboInfo.renderPass = m_renderpass;
    FboInfo.attachmentCount = 2;
    FboInfo.pAttachments = frameBufferAttachments;
    FboInfo.width = m_width;
    FboInfo.height = m_height;
    FboInfo.layers = 1;

    if (vkCreateFramebuffer(Vulkan::GetDevice(), &FboInfo, nullptr, &m_frameBuffer) != VK_SUCCESS) {
        std::cout << "error: failed to create framebuffer" << std::endl;
        return ;
    }
}

uint32_t VulkanRenderer3D::CreateVertexBuffer(const RenderSys::VertexBuffer& bufferData, RenderSys::VertexBufferLayout bufferLayout)
{
    std::cout << "Creating vertex buffer..." << std::endl;
    const auto bufferLength = bufferData.vertices.size() * sizeof(RenderSys::Vertex);
    assert(bufferLength > 0);
    assert(bufferLayout.arrayStride > 0);
    const uint64_t vertexCount = bufferLength/bufferLayout.arrayStride;
    assert(vertexCount > 0);

    VulkanVertexIndexBufferInfo vertexIndexBufferInfo;
    vertexIndexBufferInfo.m_vertexCount = vertexCount;
    vertexIndexBufferInfo.m_vertextBindingDescs.binding = 0;
    vertexIndexBufferInfo.m_vertextBindingDescs.stride = bufferLayout.arrayStride;
    vertexIndexBufferInfo.m_vertextBindingDescs.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    for (size_t i = 0; i < bufferLayout.attributeCount; i++)
    {
        RenderSys::VertexAttribute attrib = bufferLayout.attributes[i];
        VkVertexInputAttributeDescription vkAttribute{};
        vkAttribute.binding = 0;
        vkAttribute.location = attrib.location;
        vkAttribute.format = RenderSysFormatToVulkanFormat(attrib.format);
        vkAttribute.offset = attrib.offset;
        vertexIndexBufferInfo.m_vertextAttribDescs.push_back(vkAttribute);
    }

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferLength;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &vertexIndexBufferInfo.m_vertexBuffer, &vertexIndexBufferInfo.m_vertexBufferMemory, nullptr) != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return 0;
    }

    // copy data to the buffer
    void *buf;
    auto res = vmaMapMemory(m_vma, vertexIndexBufferInfo.m_vertexBufferMemory, &buf);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return 0;
    }

    std::memcpy(buf, bufferData.vertices.data(), bufferLength);
    vmaUnmapMemory(m_vma, vertexIndexBufferInfo.m_vertexBufferMemory);

    std::cout << "Vertex buffer: " << vertexIndexBufferInfo.m_vertexBuffer << std::endl;
    return m_vertexIndexBufferInfoMap.insert({m_vertexIndexBufferInfoMap.size() + 1, vertexIndexBufferInfo}).first->first;
}

void VulkanRenderer3D::CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;
    assert(bufferData.size() > 0);
    assert(sizeof(bufferData[0]) == 4); // because we are using type - VK_INDEX_TYPE_UINT32

    auto& vertexIndexBufferInfoIter = m_vertexIndexBufferInfoMap.find(vertexBufferID);
    if (vertexIndexBufferInfoIter == m_vertexIndexBufferInfoMap.end())
    {
        std::cout << "Error: could not find vertexIndexBufferInfo!" << std::endl;
        assert(false);
        return;
    }
    VulkanVertexIndexBufferInfo& vertexIndexBufferInfo = vertexIndexBufferInfoIter->second;
    vertexIndexBufferInfo.m_indexCount = bufferData.size();
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertexIndexBufferInfo.m_indexCount * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    auto res = vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &vertexIndexBufferInfo.m_indexBuffer, &vertexIndexBufferInfo.m_indexBufferMemory, nullptr);
    if (res != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return;
    }

    // copy data to the buffer
    void *buf;
    res = vmaMapMemory(m_vma, vertexIndexBufferInfo.m_indexBufferMemory, &buf);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    std::memcpy(buf, bufferData.data(), bufferInfo.size);
    vmaUnmapMemory(m_vma, vertexIndexBufferInfo.m_indexBufferMemory);
    std::cout << "Index buffer: " << vertexIndexBufferInfo.m_indexBuffer << std::endl;
}

void VulkanRenderer3D::SetClearColor(glm::vec4 clearColor)
{
    m_clearColor = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};
}

void VulkanRenderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform, uint32_t uniformCountInBuffer)
{
    const auto& [uniformBufferIter, inserted] = m_uniformBuffers.insert(
                                                    {
                                                        binding, 
                                                        std::make_tuple(
                                                            VkDescriptorBufferInfo{VK_NULL_HANDLE, 0, sizeOfOneUniform}, 
                                                            VK_NULL_HANDLE, 
                                                            nullptr
                                                        )
                                                    }
                                                );
    if (!inserted)
    {
        return;
    }

    auto& uniformBufferTuple = (*uniformBufferIter).second;
    VkDescriptorBufferInfo& bufferInfo = std::get<0>(uniformBufferTuple);
    VmaAllocation& uniformBufferMemory = std::get<1>(uniformBufferTuple);
    auto* mappedBuffer = std::get<2>(uniformBufferTuple);
    static_assert(std::is_same_v<decltype(mappedBuffer), void*>);

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeOfOneUniform;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    auto res = vmaCreateBuffer(m_vma, &bufferCreateInfo, &vmaAllocInfo, &bufferInfo.buffer, &uniformBufferMemory, nullptr);
    if (res != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return;
    }
}

void VulkanRenderer3D::SetUniformData(uint32_t binding, const void* bufferData)
{
    auto uniformBufferIter = m_uniformBuffers.find(binding);
    if (uniformBufferIter == m_uniformBuffers.end())
    {
        assert(false);
    }
    auto& uniformBufferTuple = (*uniformBufferIter).second;
    VkDescriptorBufferInfo& bufferInfo = std::get<0>(uniformBufferTuple);
    auto& uniformBufferMemory = std::get<1>(uniformBufferTuple);
    auto* mappedBuffer = std::get<2>(uniformBufferTuple);
    const auto sizeOfOneUniform = bufferInfo.range;

    auto res = vmaMapMemory(m_vma, uniformBufferMemory, &mappedBuffer);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    // copy data to the buffer
    auto* ptr = static_cast<char*>(mappedBuffer);
    memcpy(static_cast<void*>(ptr), bufferData, sizeOfOneUniform);

    vmaUnmapMemory(m_vma, uniformBufferMemory);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_mainBindGroup;
    descriptorWrite.dstBinding = binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr; // Optional
    

    vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanRenderer3D::BindResources()
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{{ 0, 0 }, { m_width, m_height }};
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

    assert(m_vertexIndexBufferInfoMap.size() == 1);
    for (auto [id, vertexIndexBufferInfo] : m_vertexIndexBufferInfoMap)
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vertexIndexBufferInfo.m_vertexBuffer, &offset);
        if (vertexIndexBufferInfo.m_indexCount > 0)
        {
            assert(vertexIndexBufferInfo.m_indexBuffer != VK_NULL_HANDLE);
            vkCmdBindIndexBuffer(m_commandBuffer, vertexIndexBufferInfo.m_indexBuffer, offset, VK_INDEX_TYPE_UINT32);
        }
    }
    
}

void VulkanRenderer3D::Render()
{
    RenderIndexed();
}

void VulkanRenderer3D::RenderIndexed()
{
    if (m_mainBindGroup)
    {
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0
                                    , 1, &m_mainBindGroup, 0, nullptr);
    }

    assert(m_vertexIndexBufferInfoMap.size() == 1);
    for (auto [id, vertexIndexBufferInfo] : m_vertexIndexBufferInfoMap)
    {
        if (vertexIndexBufferInfo.m_indexCount > 0)
        {
            vkCmdDrawIndexed(m_commandBuffer, vertexIndexBufferInfo.m_indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(m_commandBuffer, vertexIndexBufferInfo.m_vertexCount, 1, 0, 0);
        }
    }
}

void VulkanRenderer3D::RenderMesh(const RenderSys::Mesh& mesh)
{
    assert(m_mainBindGroup != VK_NULL_HANDLE);
    std::vector<VkDescriptorSet> descriptorsets{m_mainBindGroup, m_mainBindGroup};
    for (const auto &primitive : mesh.primitives)
    {
        if (primitive.materialIndex < m_materialBindGroups.size())
        {
            descriptorsets[1] = m_materialBindGroups[primitive.materialIndex];
        }

        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0
                                    , descriptorsets.size(), descriptorsets.data()
                                    , 0, nullptr);

        vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &primitive.materialIndex);

        assert(mesh.vertexBufferID >= 1);
        auto& vertexIndexBufferInfoIter = m_vertexIndexBufferInfoMap.find(mesh.vertexBufferID);
        if (vertexIndexBufferInfoIter == m_vertexIndexBufferInfoMap.end())
        {
            std::cout << "Error: could not find vertexIndexBufferInfo!" << std::endl;
            assert(false);
            return;
        }

        VulkanVertexIndexBufferInfo& vertexIndexBufferInfo = vertexIndexBufferInfoIter->second;
        if (vertexIndexBufferInfo.m_indexCount > 0)
        {
            vkCmdDrawIndexed(m_commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
        }
        else
        {
            vkCmdDraw(m_commandBuffer, vertexIndexBufferInfo.m_vertexCount, 1, 0, 0);
        }
    }
}

ImTextureID VulkanRenderer3D::GetDescriptorSet()
{
    return m_descriptorSet;
}

void VulkanRenderer3D::BeginRenderPass()
{
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = m_clearColor;
    clearValues[1].depthStencil = {1.0f, 0};

    auto err = vkResetCommandBuffer(m_commandBuffer, 0);
    Vulkan::check_vk_result(err);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(m_commandBuffer, &begin_info);
    Vulkan::check_vk_result(err);

    assert(m_frameBuffer);
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_renderpass;
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = { m_width, m_height };
    rpInfo.framebuffer = m_frameBuffer;
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer3D::EndRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
    SubmitCommandBuffer();
}

void VulkanRenderer3D::DestroyTextures()
{
    for (auto& texture : m_textures)
    {
        auto& textureTuple = texture.second;
        if (std::get<2>(textureTuple).imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(Vulkan::GetDevice(), std::get<2>(textureTuple).imageView, nullptr);
        }
        if (std::get<0>(textureTuple) != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_vma, std::get<0>(textureTuple), std::get<1>(textureTuple));
        }
    }
    m_textures.clear();
    

    if (m_defaultTextureSampler)
    {
        vkDestroySampler(Vulkan::GetDevice(), m_defaultTextureSampler, nullptr);
        m_defaultTextureSampler = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::Destroy()
{
    DestroyShaders();

    DestroyBindGroup();
    DestroyPipeline();
    DestroyBuffers();
    DestroyImages();
    DestroyTextures();

    DestroyRenderPass();

    // Destroy VMA instance
    vmaDestroyAllocator(m_vma);
}

void VulkanRenderer3D::SubmitCommandBuffer()
{
    auto err = vkEndCommandBuffer(m_commandBuffer);
    Vulkan::check_vk_result(err);

    VkSubmitInfo end_info{};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &m_commandBuffer;
    GraphicsAPI::Vulkan::QueueSubmit(end_info);
}

void VulkanRenderer3D::CreateTexture(uint32_t binding, const RenderSys::TextureDescriptor& texDescriptor)
{
    const auto& [textureIter, inserted] = m_textures.insert(
                                                    {
                                                        binding, 
                                                        std::make_tuple(
                                                            VK_NULL_HANDLE,
                                                            VK_NULL_HANDLE,
                                                            VkDescriptorImageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED}
                                                        )
                                                    }
                                                );
    if (!inserted)
    {
        return;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent.width = static_cast<uint32_t>(texDescriptor.width);
    imageInfo.extent.height = static_cast<uint32_t>(texDescriptor.height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = texDescriptor.mipMapLevelCount;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imageAllocInfo = {};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    auto& textureTuple = (*textureIter).second;
    VkImage& image = std::get<0>(textureTuple);
    VmaAllocation& imageMemory = std::get<1>(textureTuple);
    if (vmaCreateImage(m_vma, &imageInfo, &imageAllocInfo, &image, &imageMemory, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkDescriptorImageInfo& descriptorImageInfo = std::get<2>(textureTuple);
    descriptorImageInfo.imageView = CreateImageView(image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    UploadTexture(image, texDescriptor);
}

void VulkanRenderer3D::CreateTextures(const std::vector<RenderSys::TextureDescriptor>& texDescriptors)
{
    if (m_sceneTextures.size() > 0)
        return;
    
    for (const auto &texDescriptor : texDescriptors)
    {
        auto& sceneTextureTuple = m_sceneTextures.emplace_back();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.extent.width = static_cast<uint32_t>(texDescriptor.width);
        imageInfo.extent.height = static_cast<uint32_t>(texDescriptor.height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = texDescriptor.mipMapLevelCount;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo imageAllocInfo = {};
        imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage& image = std::get<0>(sceneTextureTuple);
        VmaAllocation& imageMemory = std::get<1>(sceneTextureTuple);
        if (vmaCreateImage(m_vma, &imageInfo, &imageAllocInfo, &image, &imageMemory, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkDescriptorImageInfo& descriptorImageInfo = std::get<2>(sceneTextureTuple);
        descriptorImageInfo.imageView = CreateImageView(image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        UploadTexture(image, texDescriptor);
    }

    std::cout << "VulkanRenderer3D::CreateTextures - Created " << m_sceneTextures.size() << " textures" << std::endl;
}

void VulkanRenderer3D::CreateMaterialBindGroups(const std::vector<RenderSys::Material>& materials)
{
    std::vector<VkDescriptorSetLayoutBinding> materialBindGroupBindings{
        VkDescriptorSetLayoutBinding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // baseColor texture
        VkDescriptorSetLayoutBinding{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // normal texture
        VkDescriptorSetLayoutBinding{2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}  // metallic=roughness texture
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = materialBindGroupBindings.size();
    layoutInfo.pBindings = materialBindGroupBindings.data();

    if (vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &layoutInfo, nullptr, &m_materialBindGroupLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.emplace_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * static_cast<uint32_t>(materials.size())});

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = materials.size();
    VkDescriptorPool materialBindGroupPool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &materialBindGroupPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    
    m_materialBindGroups.resize(materials.size());
    int count = 0;
    for (const auto &material : materials)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = materialBindGroupPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_materialBindGroupLayout;

        if (vkAllocateDescriptorSets(Vulkan::GetDevice(), &allocInfo, &m_materialBindGroups[count]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.resize(materialBindGroupBindings.size());
        VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
        for (size_t i = 0; i < materialBindGroupBindings.size(); i++)
        {
            VkWriteDescriptorSet textureWrite{};
            textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            textureWrite.dstSet = m_materialBindGroups[count];
            textureWrite.dstBinding = i;
            textureWrite.dstArrayElement = 0;
            textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureWrite.descriptorCount = 1;
            textureWrite.pBufferInfo = nullptr;
            textureWrite.pImageInfo = &imageInfo;
            textureWrite.pTexelBufferView = nullptr;

            descriptorWrites[i] = textureWrite;
        }

        assert(material.baseColorTextureIndex >= 0);
        VkDescriptorImageInfo& baseColorTextureImageInfo = std::get<2>(m_sceneTextures[material.baseColorTextureIndex]);
        baseColorTextureImageInfo.sampler = m_sceneTextureSamplers[0];
        descriptorWrites[0].pImageInfo = &baseColorTextureImageInfo;

        if (material.normalTextureIndex >= 0)
        {
            VkDescriptorImageInfo& normalTextureImageInfo = std::get<2>(m_sceneTextures[material.normalTextureIndex]);
            normalTextureImageInfo.sampler = m_sceneTextureSamplers[0];
            descriptorWrites[1].pImageInfo = &normalTextureImageInfo;
        }
        else
        {
            descriptorWrites[1].pImageInfo = &baseColorTextureImageInfo;
        }

        if (material.metallicRoughnessTextureIndex >= 0)
        {
            VkDescriptorImageInfo& metallicRoughnessTextureImageInfo = std::get<2>(m_sceneTextures[material.metallicRoughnessTextureIndex]);
            metallicRoughnessTextureImageInfo.sampler = m_sceneTextureSamplers[0];
            descriptorWrites[2].pImageInfo = &metallicRoughnessTextureImageInfo;
        }
        else
        {
            descriptorWrites[2].pImageInfo = &baseColorTextureImageInfo;
        }

        vkUpdateDescriptorSets(Vulkan::GetDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
        count++;
    }
}

void VulkanRenderer3D::UploadTexture(VkImage texture, const RenderSys::TextureDescriptor& texDescriptor)
{
    // Transition Image to a copyable Layout
    TransitionImageLayout(texture, VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, m_commandPool);

    uint32_t mipLevelWidth = texDescriptor.width;
    uint32_t mipLevelHeight = texDescriptor.height;
    uint32_t previousMipLevelWidth = 0;
    std::vector<uint8_t> previousLevelPixels;

    uint32_t mipMapLevelCount = 1; // TODO - Fix mipmap generation (texDescriptor.mipMapLevelCount)
    for (uint32_t level = 0; level < mipMapLevelCount; level++)
    {
        // Create image data
        const auto sizeOfData = 4 * mipLevelWidth * mipLevelHeight;
        std::vector<uint8_t> pixels(sizeOfData);
        if (level == 0) 
        {
            memcpy(pixels.data(), texDescriptor.data, sizeOfData);
        } 
		else
        {
            // Create mip level data
            for (uint32_t i = 0; i < mipLevelWidth; ++i)
            {
                for (uint32_t j = 0; j < mipLevelHeight; ++j)
                {
                    uint8_t *p = &pixels[4 * (j * mipLevelWidth + i)];
                    // Get the corresponding 4 pixels from the previous level
                    uint8_t *p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelWidth + (2 * i + 0))];
                    uint8_t *p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelWidth + (2 * i + 1))];
                    uint8_t *p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelWidth + (2 * i + 0))];
                    uint8_t *p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelWidth + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                    p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                }
            }
        }

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        CreateBuffer(m_vma, pixels.data(), static_cast<VkDeviceSize>(pixels.size()),
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, 
                    stagingBuffer, stagingBufferAllocation);

        auto currentCommandBuffer = BeginSingleTimeCommands(m_commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {mipLevelWidth, mipLevelHeight, 1};

        vkCmdCopyBufferToImage(currentCommandBuffer, stagingBuffer, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndSingleTimeCommands(currentCommandBuffer, m_commandPool);

        vmaDestroyBuffer(m_vma, stagingBuffer, stagingBufferAllocation);

        previousLevelPixels = std::move(pixels);
        previousMipLevelWidth = mipLevelWidth;
        mipLevelWidth /= 2;
        mipLevelHeight /= 2;
    }

    // Transition Image to Shader Readable Layout
    TransitionImageLayout(texture, VK_FORMAT_R8G8B8A8_SRGB, 
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, m_commandPool);
}

void VulkanRenderer3D::CreateDefaultTextureSampler()
{
    if (m_defaultTextureSampler)
        return;

    VkSamplerCreateInfo texSamplerInfo{};
    texSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    texSamplerInfo.magFilter = VK_FILTER_LINEAR;
    texSamplerInfo.minFilter = VK_FILTER_LINEAR;
    texSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    texSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    texSamplerInfo.compareEnable = VK_FALSE;
    texSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    texSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    texSamplerInfo.mipLodBias = 0.0f;
    texSamplerInfo.minLod = 0.0f;
    texSamplerInfo.maxLod = 0.0f;
    texSamplerInfo.anisotropyEnable = VK_FALSE;
    texSamplerInfo.maxAnisotropy = 1.0f;

    if (vkCreateSampler(Vulkan::GetDevice(), &texSamplerInfo, nullptr, &m_defaultTextureSampler) != VK_SUCCESS) {
        std::cout << "error: could not create sampler for texture" << std::endl;
    }
}

void VulkanRenderer3D::CreateTextureSamplers(const std::vector<RenderSys::TextureSampler>& samplers)
{
    for (const auto &sampler : samplers)
    {
        VkSamplerCreateInfo texSamplerInfo{};
        texSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        texSamplerInfo.magFilter = sampler.magFilter == RenderSys::SamplerFilterMode::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        texSamplerInfo.minFilter = sampler.minFilter == RenderSys::SamplerFilterMode::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        texSamplerInfo.addressModeU = sampler.addressModeU == RenderSys::SamplerAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texSamplerInfo.addressModeV = sampler.addressModeV == RenderSys::SamplerAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texSamplerInfo.addressModeW = sampler.addressModeW == RenderSys::SamplerAddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        texSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        texSamplerInfo.unnormalizedCoordinates = VK_FALSE;
        texSamplerInfo.compareEnable = VK_FALSE;
        texSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        texSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        texSamplerInfo.mipLodBias = 0.0f;
        texSamplerInfo.minLod = 0.0f;
        texSamplerInfo.maxLod = 0.0f;
        texSamplerInfo.anisotropyEnable = VK_FALSE;
        texSamplerInfo.maxAnisotropy = 1.0f;

        VkSampler textureSampler = VK_NULL_HANDLE;
        if (vkCreateSampler(Vulkan::GetDevice(), &texSamplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            std::cout << "error: could not create sampler for texture" << std::endl;
        }

        m_sceneTextureSamplers.push_back(textureSampler);
    }
}

} // namespace GraphicsAPI