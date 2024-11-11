#include "VulkanRenderer2D.h"
#include "VulkanRendererUtils.h"

#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace GraphicsAPI
{

bool VulkanRenderer2D::Init()
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

    if (!CreateRenderPass())
    {
        return false;
    }

    return true;
}

void VulkanRenderer2D::CreateTextureToRenderInto(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    static VkDeviceMemory m_Memory;
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_UNORM;
    info.extent.width = m_width;
    info.extent.height = m_height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult err = vkCreateImage(Vulkan::GetDevice(), &info, nullptr, &m_ImageToRenderInto);
    Vulkan::check_vk_result(err);
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(Vulkan::GetDevice(), m_ImageToRenderInto, &req);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(Vulkan::GetPhysicalDevice(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
    err = vkAllocateMemory(Vulkan::GetDevice(), &alloc_info, nullptr, &m_Memory);
    Vulkan::check_vk_result(err);
    err = vkBindImageMemory(Vulkan::GetDevice(), m_ImageToRenderInto, m_Memory, 0);
    Vulkan::check_vk_result(err);

    VkImageViewCreateInfo viewinfo = {};
    viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewinfo.image = m_ImageToRenderInto;
    viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewinfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewinfo.subresourceRange.levelCount = 1;
    viewinfo.subresourceRange.layerCount = 1;
    err = vkCreateImageView(Vulkan::GetDevice(), &viewinfo, nullptr, &m_imageViewToRenderInto);
    Vulkan::check_vk_result(err);

    CreateTextureSampler();
    m_descriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_textureSampler, m_imageViewToRenderInto, VK_IMAGE_LAYOUT_GENERAL);


    CreateFrameBuffer();
}

void VulkanRenderer2D::CreateShaders(RenderSys::Shader& shader)
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
    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = shaderStageBits;
    vertexStageInfo.module = shaderModule;
    vertexStageInfo.pName = "main";

    m_shaderStageInfos.push_back(vertexStageInfo);
}

void VulkanRenderer2D::DestroyBuffers()
{
    if (m_vertexBuffer != VK_NULL_HANDLE && m_vertexBufferMemory != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vma, m_vertexBuffer, m_vertexBufferMemory);
    }

    if (m_indexBuffer != VK_NULL_HANDLE && m_indexBufferMemory != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vma, m_indexBuffer, m_indexBufferMemory);
    }
}

void VulkanRenderer2D::DestroyShaders()
{
    for (auto& shaderStageInfo : m_shaderStageInfos)
    {
        vkDestroyShaderModule(Vulkan::GetDevice(), shaderStageInfo.module, nullptr);
    }

    m_shaderStageInfos.clear();
}

void VulkanRenderer2D::CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount)
{
    CreateShaders(shader);
    m_vertexCount = vertexShaderCallCount;
}

void VulkanRenderer2D::SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    // Create a bind group layout
    if (!m_bindGroupLayout)
    {
        auto& uboLayoutBinding = m_bindGroupBindings.emplace_back();
        uboLayoutBinding.binding = 0;
        if (bindGroupLayoutEntry.buffer.hasDynamicOffset)
        {
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }
        else
        {
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = GetVulkanShaderStageVisibility(bindGroupLayoutEntry.visibility);
        uboLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(Vulkan::GetDevice(), &layoutInfo, nullptr, &m_bindGroupLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        VkDescriptorPoolSize poolSize{};
        poolSize.type = uboLayoutBinding.descriptorType;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_bindGroupPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
}

void VulkanRenderer2D::CreateBindGroup()
{
    if (!m_bindGroup)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_bindGroupPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_bindGroupLayout;

        if (vkAllocateDescriptorSets(Vulkan::GetDevice(), &allocInfo, &m_bindGroup) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }
    else
    {
        //std::cout << "No bind group layout" << std::endl;
    }
}

void VulkanRenderer2D::CreatePipelineLayout()
{
    if (!m_pipelineLayout)
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        if (!m_bindGroupLayout)
        {
            pipelineLayoutCreateInfo.setLayoutCount = 0;
            pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        }
        else
        {
            pipelineLayoutCreateInfo.setLayoutCount = 1;
            pipelineLayoutCreateInfo.pSetLayouts = &m_bindGroupLayout;
        }
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(Vulkan::GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            std::cout << "error: could not create pipeline layout" << std::endl;
        }        
    }
}

bool VulkanRenderer2D::CreateRenderPass()
{
    assert(!m_renderpass);

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
    // subpassDesc.pDepthStencilAttachment = &depthAttRef;

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
    VkAttachmentDescription attachments[] = {colorAtt};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    //   renderPassInfo.dependencyCount = 2;
    //   renderPassInfo.pDependencies = dependencies;

    if (vkCreateRenderPass(Vulkan::GetDevice(), &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
    {
        std::cout << "error; could not create renderpass" << std::endl;
        return false;
    }

    return true;
}

void VulkanRenderer2D::CreatePipeline()
{
    std::cout << "Creating render pipeline..." << std::endl;

    /* assemble the graphics pipeline itself */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = m_vertextBindingDescs.size();
    vertexInputInfo.pVertexBindingDescriptions = m_vertextBindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = m_vertextAttribDescs.size();
    vertexInputInfo.pVertexAttributeDescriptions = m_vertextAttribDescs.data();

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
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        vkDestroyPipelineLayout(Vulkan::GetDevice(), m_pipelineLayout, nullptr);
    }

    /* it is save to destroy the shader modules after pipeline has been created */
    DestroyShaders();
    
    std::cout << "Render pipeline: " << m_pipeline << std::endl;
}

void VulkanRenderer2D::CreateFrameBuffer()
{
    if (m_frameBuffer)
    {
        vkDestroyFramebuffer(Vulkan::GetDevice(), m_frameBuffer, nullptr);
    }

    VkImageView frameBufferAttachments[] = { m_imageViewToRenderInto };
    VkFramebufferCreateInfo FboInfo{};
    FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FboInfo.renderPass = m_renderpass;
    FboInfo.attachmentCount = 1;
    FboInfo.pAttachments = frameBufferAttachments;
    FboInfo.width = m_width;
    FboInfo.height = m_height;
    FboInfo.layers = 1;

    if (vkCreateFramebuffer(Vulkan::GetDevice(), &FboInfo, nullptr, &m_frameBuffer) != VK_SUCCESS) {
        std::cout << "error: failed to create framebuffer" << std::endl;
        return ;
    }
}

void VulkanRenderer2D::CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
{
    std::cout << "Creating vertex buffer..." << std::endl;

    assert(bufferLayout.arrayStride > 0);
    m_vertexCount = bufferLength/bufferLayout.arrayStride;
    assert(m_vertexCount > 0);
    
    static bool vertBufCreated = false;
    if (!vertBufCreated)
    {
        VkVertexInputBindingDescription mainBinding{};
        mainBinding.binding = 0;
        mainBinding.stride = bufferLayout.arrayStride;
        mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        m_vertextBindingDescs.push_back(mainBinding);

        for (size_t i = 0; i < bufferLayout.attributeCount; i++)
        {
            RenderSys::VertexAttribute attrib = bufferLayout.attributes[i];
            VkVertexInputAttributeDescription vkAttribute{};
            vkAttribute.binding = 0;
            vkAttribute.location = attrib.location;
            vkAttribute.format = RenderSysFormatToVulkanFormat(attrib.format);
            vkAttribute.offset = attrib.offset;

            m_vertextAttribDescs.push_back(vkAttribute);
        }

        // VkVertexInputAttributeDescription uvAttribute{};
        // uvAttribute.binding = 0;
        // uvAttribute.location = 1;
        // uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
        // uvAttribute.offset = offsetof(RenderSysVkVertex, uv);

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferLength;
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        if (vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &m_vertexBuffer, &m_vertexBufferMemory, nullptr) != VK_SUCCESS) {
            std::cout << "vkCreateBuffer() failed!" << std::endl;
            return;
        }

        // copy data to the buffer
		void *buf;
		auto res = vmaMapMemory(m_vma, m_vertexBufferMemory, &buf);
		if (res != VK_SUCCESS) {
			std::cout << "vkMapMemory() failed" << std::endl;
			return;
		}

		std::memcpy(buf, bufferData, bufferLength);
		vmaUnmapMemory(m_vma, m_vertexBufferMemory);
        
        vertBufCreated = true;
    }

    std::cout << "Vertex buffer: " << m_vertexBuffer << std::endl;
}

void VulkanRenderer2D::CreateIndexBuffer(const std::vector<uint16_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;

    m_indexCount = bufferData.size();
    assert(m_indexCount > 0);
    
    static bool indexBufCreated = false;
    if (!indexBufCreated)
    {
        VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_indexCount * sizeof(bufferData[0]);
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VmaAllocationCreateInfo vmaAllocInfo{};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        auto res = vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &m_indexBuffer, &m_indexBufferMemory, nullptr);
		if (res != VK_SUCCESS) {
			std::cout << "vkCreateBuffer() failed!" << std::endl;
			return;
		}

        // copy data to the buffer
		void *buf;
		res = vmaMapMemory(m_vma, m_indexBufferMemory, &buf);
		if (res != VK_SUCCESS) {
			std::cout << "vkMapMemory() failed" << std::endl;
			return;
		}

		std::memcpy(buf, bufferData.data(), bufferInfo.size);
		vmaUnmapMemory(m_vma, m_indexBufferMemory);
        
        indexBufCreated = true;
    }

    std::cout << "Index buffer: " << m_indexBuffer << std::endl;
}

uint32_t VulkanRenderer2D::GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform)
{
    if (uniformIndex == 0)
        return 0;

    static VkDeviceSize minUniformBufferOffsetAlignment = 0;
    if (minUniformBufferOffsetAlignment == 0)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(Vulkan::GetPhysicalDevice(), &deviceProperties);
        minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        std::cout << "VulkanRenderer2D::GetUniformStride - " << minUniformBufferOffsetAlignment << std::endl;
    }

    assert(sizeOfUniform > 0);
    auto ceilToNextMultiple = [](uint32_t value, uint32_t step) -> uint32_t
    {
        uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
        return step * divide_and_ceil;
    };

    uint32_t uniformStride = ceilToNextMultiple(
        (uint32_t)sizeOfUniform,
        (uint32_t)minUniformBufferOffsetAlignment
    );

    return uniformStride;
}

void VulkanRenderer2D::CreateUniformBuffer(size_t uniformCountInBuffer, uint32_t sizeOfOneUniform)
{
    m_sizeOfOneUniform = sizeOfOneUniform;
    m_uniformBuffers.resize(1);
    m_uniformBuffersMemory.resize(1);
    m_uniformBuffersMapped.resize(1);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const size_t maximumUniformIndex = uniformCountInBuffer - 1;
    bufferInfo.size = m_sizeOfOneUniform + GetUniformStride(maximumUniformIndex, m_sizeOfOneUniform);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    auto res = vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &m_uniformBuffers[0], &m_uniformBuffersMemory[0], nullptr);
    if (res != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return;
    }
}

void VulkanRenderer2D::SetUniformData(const void* bufferData, uint32_t uniformIndex)
{
    assert(m_uniformBuffers[0]);
    assert(m_sizeOfOneUniform > 0);

    auto res = vmaMapMemory(m_vma, m_uniformBuffersMemory[0], &m_uniformBuffersMapped[0]);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    auto offset = GetUniformStride(uniformIndex, m_sizeOfOneUniform);
    // copy data to the buffer
    auto* ptr = static_cast<char*>(m_uniformBuffersMapped[0]) + offset;
    memcpy(static_cast<void*>(ptr), bufferData, m_sizeOfOneUniform);

    vmaUnmapMemory(m_vma, m_uniformBuffersMemory[0]);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_uniformBuffers[0];
    bufferInfo.offset = 0;
    bufferInfo.range = m_sizeOfOneUniform;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_bindGroup;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    assert(m_bindGroupBindings.size() > 0);
    descriptorWrite.descriptorType = m_bindGroupBindings[0].descriptorType; //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanRenderer2D::SimpleRender()
{
    vkCmdBindPipeline(m_commandBufferForReal, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBufferForReal, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { m_width, m_height };
    vkCmdSetScissor(m_commandBufferForReal, 0, 1, &scissor);

    const auto triangleCount = 1;
    vkCmdDraw(m_commandBufferForReal, triangleCount * 3, 1, 0, 0);
}

void VulkanRenderer2D::Render()
{
    vkCmdBindPipeline(m_commandBufferForReal, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBufferForReal, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { m_width, m_height };
    vkCmdSetScissor(m_commandBufferForReal, 0, 1, &scissor);

    VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBufferForReal, 0, 1, &m_vertexBuffer, &offset);
    vkCmdDraw(m_commandBufferForReal, m_vertexCount, 1, 0, 0);
}

void VulkanRenderer2D::RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount)
{
    vkCmdBindPipeline(m_commandBufferForReal, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_width);
    viewport.height = static_cast<float>(m_height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_commandBufferForReal, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { m_width, m_height };
    vkCmdSetScissor(m_commandBufferForReal, 0, 1, &scissor);

    VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBufferForReal, 0, 1, &m_vertexBuffer, &offset);
    vkCmdBindIndexBuffer(m_commandBufferForReal, m_indexBuffer, offset, VK_INDEX_TYPE_UINT16);

    if (m_bindGroup)
    {
        uint32_t dynamicOffset = 0;
        if (uniformIndex > 0)
            dynamicOffset = GetUniformStride(uniformIndex, m_sizeOfOneUniform);
        vkCmdBindDescriptorSets(m_commandBufferForReal, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_bindGroup, dynamicOffsetCount, &dynamicOffset);
    }

    vkCmdDrawIndexed(m_commandBufferForReal, m_indexCount, 1, 0, 0, 0);
}

void VulkanRenderer2D::CreateTextureSampler()
{
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

    if (vkCreateSampler(Vulkan::GetDevice(), &texSamplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
        std::cout << "error: could not create sampler for texture" << std::endl;
    }
}

ImTextureID VulkanRenderer2D::GetDescriptorSet()
{
    return m_descriptorSet;
}

void VulkanRenderer2D::BeginRenderPass()
{
    VkClearValue colorClearValue;
    colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { colorClearValue, depthValue };

    m_commandBufferForReal = GraphicsAPI::Vulkan::GetCommandBuffer(true); // CRITICAL: only call once in the renderer

    assert(m_frameBuffer);
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_renderpass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = { m_width, m_height };
    rpInfo.framebuffer = m_frameBuffer;
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(m_commandBufferForReal, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer2D::EndRenderPass()
{
	vkCmdEndRenderPass(m_commandBufferForReal);
    SubmitCommandBuffer();
}

void VulkanRenderer2D::Destroy()
{
    DestroyBuffers();

    // Destroy VMA instance
    vmaDestroyAllocator(m_vma);
}

void VulkanRenderer2D::SubmitCommandBuffer()
{
    GraphicsAPI::Vulkan::FlushCommandBuffer(m_commandBufferForReal);

    // VkSubmitInfo submitInfo{};
    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // submitInfo.pWaitDstStageMask = &waitStage;
    // submitInfo.commandBufferCount = 1;
    // submitInfo.pCommandBuffers = &m_commandBufferForReal;

    // if (vkQueueSubmit(Vulkan::GetDeviceQueue(), 1, &submitInfo, g_renderFence) != VK_SUCCESS) {
    //     std::cout << "error: failed to submit draw command buffer" << std::endl;
    //     return;
    // }
}

} // namespace GraphicsAPI