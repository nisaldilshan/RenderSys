#include "VulkanRenderer3D.h"
#include "VulkanRendererUtils.h"

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

    if (!CreateRenderPass())
    {
        return false;
    }

    return true;
}

void VulkanRenderer3D::CreateTextureToRenderInto(uint32_t width, uint32_t height)
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
    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = shaderStageBits;
    vertexStageInfo.module = shaderModule;
    vertexStageInfo.pName = "main";

    m_shaderStageInfos.push_back(vertexStageInfo);
}

void VulkanRenderer3D::DestroyBuffers()
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

void VulkanRenderer3D::DestroyShaders()
{
    for (auto& shaderStageInfo : m_shaderStageInfos)
    {
        vkDestroyShaderModule(Vulkan::GetDevice(), shaderStageInfo.module, nullptr);
    }

    m_shaderStageInfos.clear();
}

void VulkanRenderer3D::CreateStandaloneShader(RenderSys::Shader& shader, uint32_t vertexShaderCallCount)
{
    CreateShaders(shader);
    m_vertexCount = vertexShaderCallCount;
}

void VulkanRenderer3D::CreatePipelineLayout()
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

bool VulkanRenderer3D::CreateRenderPass()
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

void VulkanRenderer3D::CreatePipeline()
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

void VulkanRenderer3D::CreateFrameBuffer()
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

void VulkanRenderer3D::CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
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

void VulkanRenderer3D::CreateIndexBuffer(const std::vector<uint16_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;

    // m_indexCount = bufferData.size();

    // wgpu::BufferDescriptor bufferDesc;
    // bufferDesc.size = bufferData.size() * sizeof(float);
    // bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
    // bufferDesc.label = "Index Buffer";
    // m_indexBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);

    // // Upload index data to the buffer
    // WebGPU::GetQueue().writeBuffer(m_indexBuffer, 0, bufferData.data(), bufferDesc.size);
    // std::cout << "Index buffer: " << m_indexBuffer << std::endl;
}

void VulkanRenderer3D::SetClearColor(glm::vec4 clearColor)
{
    // m_clearColor = wgpu::Color{clearColor.x, clearColor.y, clearColor.z, clearColor.w};
}

void VulkanRenderer3D::CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries)
{
    // // Create a bind group layout using a vector of layout entries
    // auto bindGroupLayoutEntryCount = (uint32_t)bindGroupLayoutEntries.size();
	// wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
	// bindGroupLayoutDesc.entryCount = bindGroupLayoutEntryCount;
	// bindGroupLayoutDesc.entries = bindGroupLayoutEntries.data();
    // bindGroupLayoutDesc.label = "MainBindGroupLayout";
	// m_bindGroupLayout = WebGPU::GetDevice().createBindGroupLayout(bindGroupLayoutDesc);

    // if (m_bindGroupLayout)
    // {
    //     // Create the pipeline layout
    //     wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    //     pipelineLayoutDesc.bindGroupLayoutCount = 1;
    //     pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
    //     pipelineLayoutDesc.label = "PipelineLayout";
    //     m_pipelineLayout = WebGPU::GetDevice().createPipelineLayout(pipelineLayoutDesc);

    //     assert(bindGroupLayoutEntryCount > 0);
    //     std::vector<wgpu::BindGroupEntry> bindings;
    //     bindings.resize(bindGroupLayoutEntryCount);

    //     for (const auto& bindGroupLayoutEntry : bindGroupLayoutEntries)
    //     {
    //         auto bindingIndex = bindGroupLayoutEntry.binding;
    //         bindings[bindingIndex].binding = bindingIndex;

    //         if (bindGroupLayoutEntry.buffer.type == wgpu::BufferBindingType::Uniform)
    //         {
    //             bool uniformBufferFound = false;
    //             for (const auto& uniformBuffer : m_uniformBuffers)
    //             {
    //                 auto bindingOfUniform = std::get<0>(uniformBuffer.second);
    //                 if (bindingOfUniform == bindingIndex)
    //                 {
    //                     uniformBufferFound = true;
    //                     const auto buffer = std::get<1>(uniformBuffer.second);
    //                     const auto bufferSize = std::get<2>(uniformBuffer.second);
    //                     bindings[bindingIndex].buffer = buffer;
    //                     bindings[bindingIndex].offset = 0;
    //                     bindings[bindingIndex].size = bufferSize;
    //                     break;
    //                 }
    //             }

    //             assert(uniformBufferFound);
    //         }
    //         else if (bindGroupLayoutEntry.sampler.type == wgpu::SamplerBindingType::Filtering)
    //         {
    //             bindings[bindingIndex].sampler = m_textureSampler;
    //         }
    //         else if (bindGroupLayoutEntry.texture.viewDimension == wgpu::TextureViewDimension::_2D)
    //         {
    //             if (bindingIndex == 1) // this is base color texture
    //             {
    //                 bindings[bindingIndex].textureView = m_texturesAndViews[0].second; // m_texturesAndViews[0] is the base color texture
    //             }
    //             else // this is normal texture
    //             {
    //                 bindings[bindingIndex].textureView = m_texturesAndViews[1].second; // m_texturesAndViews[0] is the normal texture
    //             }
    //         }
    //         else
    //         {
    //             assert(false);
    //         }
    //     }
        

    //     // auto modelViewProjectionUniformBuffer = m_uniformBuffers.find(Uniform::UniformType::ModelViewProjection);
    //     // if (modelViewProjectionUniformBuffer != m_uniformBuffers.end())
    //     // {
    //     //     const auto buffer = modelViewProjectionUniformBuffer->second.first;
    //     //     const auto bufferSize = modelViewProjectionUniformBuffer->second.second;
    //     //     assert(bufferSize > 0);
    //     //     bindings[0].binding = 0;
    //     //     bindings[0].buffer = buffer;
    //     //     bindings[0].offset = 0;
    //     //     bindings[0].size = bufferSize;
    //     // }

    //     // if (m_texturesAndViews.size() > 0)
    //     // {
    //     //     assert(bindGroupLayoutEntryCount > 1);
    //     //     bindings[1].binding = 1;
	//     //     bindings[1].textureView = m_texturesAndViews[0].second; // m_texturesAndViews[0] is the base color texture
    //     // }

    //     // if (m_texturesAndViews.size() > 1)
    //     // {
    //     //     assert(bindGroupLayoutEntryCount > 2);
    //     //     bindings[2].binding = 2;
    //     //     bindings[2].textureView = m_texturesAndViews[1].second;
    //     // }

    //     // if (m_textureSampler)
    //     // {
    //     //     assert(bindGroupLayoutEntryCount > 3);
    //     //     bindings[3].binding = 3;
    //     //     bindings[3].sampler = m_textureSampler;
    //     // }

    //     // auto lightingUniformBuffer = m_uniformBuffers.find(Uniform::UniformType::Lighting);
    //     // if (lightingUniformBuffer != m_uniformBuffers.end())
    //     // {
    //     //     const auto buffer = lightingUniformBuffer->second.first;
    //     //     const auto bufferSize = lightingUniformBuffer->second.second;
    //     //     assert(bufferSize > 0);
    //     //     bindings[4].binding = 4;
    //     //     bindings[4].buffer = buffer;
    //     //     bindings[4].offset = 0;
    //     //     bindings[4].size = bufferSize;
    //     // }

    //     // A bind group contains one or multiple bindings
    //     wgpu::BindGroupDescriptor bindGroupDesc;
    //     bindGroupDesc.layout = m_bindGroupLayout;
    //     // There must be as many bindings as declared in the layout!
    //     bindGroupDesc.entryCount = (uint32_t)bindings.size();
    //     bindGroupDesc.entries = bindings.data();
    //     m_bindGroup = WebGPU::GetDevice().createBindGroup(bindGroupDesc);
    // }
}

uint32_t VulkanRenderer3D::GetUniformStride(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform)
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

void VulkanRenderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform, uint32_t uniformCountInBuffer)
{
    const auto& [uniformBufferIter, inserted] = m_uniformBuffers.insert({binding, std::make_tuple(VK_NULL_HANDLE, VK_NULL_HANDLE, nullptr, sizeOfOneUniform)});
    if (!inserted)
    {
        std::cout << "VulkanRenderer3D::CreateUniformBuffer() failed!" << std::endl;
        return;
    }

    auto& uniformBufferTuple = (*uniformBufferIter).second;
    auto& uniformBuffer = std::get<0>(uniformBufferTuple);
    auto& uniformBufferMemory = std::get<1>(uniformBufferTuple);
    auto* mappedBuffer = std::get<2>(uniformBufferTuple);
    static_assert(std::is_same_v<decltype(uniformBuffer), VkBuffer&>); 
    static_assert(std::is_same_v<decltype(uniformBufferMemory), VmaAllocation&>);
    static_assert(std::is_same_v<decltype(mappedBuffer), void*>);

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    const size_t maximumUniformIndex = uniformCountInBuffer - 1;
    bufferInfo.size = sizeOfOneUniform + GetUniformStride(maximumUniformIndex, sizeOfOneUniform);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    auto res = vmaCreateBuffer(m_vma, &bufferInfo, &vmaAllocInfo, &uniformBuffer, &uniformBufferMemory, nullptr);
    if (res != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return;
    }
}

void VulkanRenderer3D::SetUniformData(uint32_t binding, const void* bufferData, uint32_t uniformIndex)
{
    auto uniformBufferIter = m_uniformBuffers.find(binding);
    if (uniformBufferIter == m_uniformBuffers.end())
    {
        assert(false);
    }
    auto& uniformBufferTuple = (*uniformBufferIter).second;
    auto& uniformBuffer = std::get<0>(uniformBufferTuple);
    auto& uniformBufferMemory = std::get<1>(uniformBufferTuple);
    auto* mappedBuffer = std::get<2>(uniformBufferTuple);
    const auto sizeOfOneUniform = std::get<3>(uniformBufferTuple);

    auto res = vmaMapMemory(m_vma, uniformBufferMemory, &mappedBuffer);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    auto offset = GetUniformStride(uniformIndex, sizeOfOneUniform);
    // copy data to the buffer
    auto* ptr = static_cast<char*>(mappedBuffer) + offset;
    memcpy(static_cast<void*>(ptr), bufferData, sizeOfOneUniform);

    vmaUnmapMemory(m_vma, uniformBufferMemory);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeOfOneUniform;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_bindGroup;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    assert(m_bindGroupBindings.size() > 0);
    descriptorWrite.descriptorType = m_bindGroupBindings[0].descriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    vkUpdateDescriptorSets(Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanRenderer3D::SimpleRender()
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

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { m_width, m_height };
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

    const auto triangleCount = 1;
    vkCmdDraw(m_commandBuffer, triangleCount * 3, 1, 0, 0);
}

void VulkanRenderer3D::Render(uint32_t uniformIndex)
{
    // // Set vertex buffer while encoding the render pass
    // m_renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBufferSize);

    // // Set binding group
    // if (m_bindGroup)
    // {
    //     uint32_t dynamicOffset = 0;
    //     auto modelViewProjectionUniformBuffer = m_uniformBuffers.find(UniformBuf::UniformType::ModelViewProjection);
    //     if (modelViewProjectionUniformBuffer != m_uniformBuffers.end())
    //     {
    //         dynamicOffset = uniformIndex * GetOffset(1, std::get<2>(modelViewProjectionUniformBuffer->second)); // TODO: better to use a array of offsets and select a offset from it
    //     }
    //     uint32_t dynamicOffsetCount = 1; // because we have enabled dynamic offset in only one binding in the bind group
    //     m_renderPass.setBindGroup(0, m_bindGroup, dynamicOffsetCount, &dynamicOffset);
    // }

    // if (m_indexCount > 0)
    // {
    //     m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexCount * sizeof(uint16_t));
    //     m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
    // }
    // else
    //     m_renderPass.draw(m_vertexCount, 1, 0, 0);
}

void VulkanRenderer3D::RenderIndexed(uint32_t uniformIndex)
{
    // assert(m_indexBuffer);
    // // Set vertex buffer while encoding the render pass
    // m_renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBufferSize);
    // // Set index buffer while encoding the render pass
    // m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexCount * sizeof(uint16_t));

    // // Set binding group
    // uint32_t dynamicOffset = 0;
    // auto modelViewProjectionUniformBuffer = m_uniformBuffers.find(UniformBuf::UniformType::ModelViewProjection);
    // if (modelViewProjectionUniformBuffer != m_uniformBuffers.end())
    // {
    //     dynamicOffset = uniformIndex * GetOffset(1, std::get<2>(modelViewProjectionUniformBuffer->second)); // TODO: better to use a array of offsets and select a offset from it
    // }
    // uint32_t dynamicOffsetCount = 1; // because we have enabled dynamic offset in only one binding in the bind group
    // m_renderPass.setBindGroup(0, m_bindGroup, dynamicOffsetCount, &dynamicOffset);
    // m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
}

ImTextureID VulkanRenderer3D::GetDescriptorSet()
{
    return m_descriptorSet;
}

void VulkanRenderer3D::BeginRenderPass()
{
    VkClearValue colorClearValue;
    colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { colorClearValue, depthValue };

    m_commandBuffer = GraphicsAPI::Vulkan::GetCommandBuffer(true); // CRITICAL: only call once in the renderer

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

    vkCmdBeginRenderPass(m_commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer3D::EndRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
    SubmitCommandBuffer();
}

void VulkanRenderer3D::Destroy()
{
    DestroyBuffers();

    // Destroy VMA instance
    vmaDestroyAllocator(m_vma);
}

void VulkanRenderer3D::SubmitCommandBuffer()
{
    GraphicsAPI::Vulkan::FlushCommandBuffer(m_commandBuffer);
}

void VulkanRenderer3D::CreateDepthTexture()
{
    // // Create the depth texture
	// wgpu::TextureDescriptor depthTextureDesc;
	// depthTextureDesc.dimension = wgpu::TextureDimension::_2D;
	// depthTextureDesc.format = m_depthTextureFormat;
	// depthTextureDesc.mipLevelCount = 1;
	// depthTextureDesc.sampleCount = 1;
	// depthTextureDesc.size = {m_width, m_height, 1};
	// depthTextureDesc.usage = wgpu::TextureUsage::RenderAttachment;
	// depthTextureDesc.viewFormatCount = 1;
	// depthTextureDesc.viewFormats = (WGPUTextureFormat*)&m_depthTextureFormat;
	// m_depthTexture = WebGPU::GetDevice().createTexture(depthTextureDesc);
	// std::cout << "Depth texture: " << m_depthTexture << std::endl;

	// // Create the view of the depth texture manipulated by the rasterizer
	// wgpu::TextureViewDescriptor depthTextureViewDesc;
	// depthTextureViewDesc.aspect = wgpu::TextureAspect::DepthOnly;
	// depthTextureViewDesc.baseArrayLayer = 0;
	// depthTextureViewDesc.arrayLayerCount = 1;
	// depthTextureViewDesc.baseMipLevel = 0;
	// depthTextureViewDesc.mipLevelCount = 1;
	// depthTextureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	// depthTextureViewDesc.format = m_depthTextureFormat;
	// m_depthTextureView = m_depthTexture.createView(depthTextureViewDesc);
	// std::cout << "Depth texture view: " << m_depthTextureView << std::endl;
}

void VulkanRenderer3D::CreateTexture(uint32_t textureWidth, uint32_t textureHeight, const void* textureData, uint32_t mipMapLevelCount)
{
	// wgpu::TextureDescriptor textureDesc;
	// textureDesc.dimension = wgpu::TextureDimension::_2D;
	// textureDesc.size = {textureWidth, textureHeight, 1};
	// textureDesc.format = wgpu::TextureFormat::RGBA8Unorm;;
	// textureDesc.mipLevelCount = mipMapLevelCount;
	// textureDesc.sampleCount = 1;
	// textureDesc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;
	// textureDesc.viewFormatCount = 0;
	// textureDesc.viewFormats = nullptr;
	// auto newTexture = WebGPU::GetDevice().createTexture(textureDesc);
	// std::cout << "texture created: " << newTexture << std::endl;

	// wgpu::TextureViewDescriptor textureViewDesc;
	// textureViewDesc.aspect = wgpu::TextureAspect::All;
	// textureViewDesc.baseArrayLayer = 0;
	// textureViewDesc.arrayLayerCount = 1;
	// textureViewDesc.baseMipLevel = 0;
	// textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
	// textureViewDesc.dimension = wgpu::TextureViewDimension::_2D;
	// textureViewDesc.format = textureDesc.format;
	// auto newTextureView = newTexture.createView(textureViewDesc);
	// std::cout << "texture view created: " << newTextureView << std::endl;

    // m_texturesAndViews.emplace_back(std::make_pair(newTexture, newTextureView));

    // UploadTexture(newTexture, textureDesc, textureData);
}

void VulkanRenderer3D::UploadTexture(VkImage texture, RenderSys::TextureDescriptor textureDesc, const void* textureData)
{
    // wgpu::ImageCopyTexture destination;
    // destination.texture = texture;
    // destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    // destination.aspect = wgpu::TextureAspect::All; // only relevant for depth/Stencil textures

    // // Arguments telling how the C++ side pixel memory is laid out
    // wgpu::TextureDataLayout source;
    // source.offset = 0;

    // wgpu::Extent3D mipLevelSize = textureDesc.size;
	// std::vector<uint8_t> previousLevelPixels;
    // wgpu::Extent3D previousMipLevelSize;
	// for (uint32_t level = 0; level < textureDesc.mipLevelCount; ++level) {
	// 	// Create image data
    //     std::vector<uint8_t> pixels(4 * mipLevelSize.width * mipLevelSize.height);
    //     if (level == 0) 
    //     {
    //         memcpy(pixels.data(), textureData, 4 * mipLevelSize.width * mipLevelSize.height);
    //     } 
	// 	else
    //     {
    //         // Create mip level data
    //         for (uint32_t i = 0; i < mipLevelSize.width; ++i)
    //         {
    //             for (uint32_t j = 0; j < mipLevelSize.height; ++j)
    //             {
    //                 uint8_t *p = &pixels[4 * (j * mipLevelSize.width + i)];
    //                 // Get the corresponding 4 pixels from the previous level
    //                 uint8_t *p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 0))];
    //                 uint8_t *p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelSize.width + (2 * i + 1))];
    //                 uint8_t *p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 0))];
    //                 uint8_t *p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelSize.width + (2 * i + 1))];
    //                 // Average
    //                 p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
    //                 p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
    //                 p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
    //                 p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
    //             }
    //         }
    //     }

    //     // Change this to the current level
    //     destination.mipLevel = level;
    //     source.bytesPerRow = 4 * mipLevelSize.width;
    //     source.rowsPerImage = mipLevelSize.height;
    //     WebGPU::GetQueue().writeTexture(destination, pixels.data(), pixels.size(), source, mipLevelSize);

    //     previousLevelPixels = std::move(pixels);
    //     previousMipLevelSize = mipLevelSize;
    //     mipLevelSize.width /= 2;
    //     mipLevelSize.height /= 2;
    // }
}

void VulkanRenderer3D::CreateTextureSampler()
{
    // wgpu::SamplerDescriptor samplerDesc;
    // samplerDesc.addressModeU = wgpu::AddressMode::Repeat;
    // samplerDesc.addressModeV = wgpu::AddressMode::Repeat;
    // samplerDesc.addressModeW = wgpu::AddressMode::Repeat;
    // samplerDesc.magFilter = wgpu::FilterMode::Linear;
    // samplerDesc.minFilter = wgpu::FilterMode::Linear;
    // samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
    // samplerDesc.lodMinClamp = 0.0f;
    // samplerDesc.lodMaxClamp = 8.0f;
    // samplerDesc.compare = wgpu::CompareFunction::Undefined;
    // samplerDesc.maxAnisotropy = 1;
    // m_textureSampler = WebGPU::GetDevice().createSampler(samplerDesc);
}

} // namespace GraphicsAPI