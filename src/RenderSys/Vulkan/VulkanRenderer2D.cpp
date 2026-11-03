#include "VulkanRenderer2D.h"
#include "Pipeline/VulkanRender2DPipeline.h"

#include <vk_mem_alloc.h>

#include <iostream>

namespace RenderSys
{

VulkanRenderer2D::VulkanRenderer2D()
{
}

VulkanRenderer2D::~VulkanRenderer2D()
{
    Destroy();
}

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
    DestroyImages();
    m_width = width;
    m_height = height;

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
    Vulkan::CheckVkResult(err);
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(Vulkan::GetDevice(), m_ImageToRenderInto, &req);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = Vulkan::FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    err = vkAllocateMemory(Vulkan::GetDevice(), &alloc_info, nullptr, &m_renderImageMemory);
    Vulkan::CheckVkResult(err);
    err = vkBindImageMemory(Vulkan::GetDevice(), m_ImageToRenderInto, m_renderImageMemory, 0);
    Vulkan::CheckVkResult(err);

    CreateTextureSampler();

    const auto imageViewToRenderInto = RenderSys::Vulkan::CreateImageView(m_ImageToRenderInto, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    m_finalRenderTarget = RenderSys::Vulkan::CreateRenderTarget(imageViewToRenderInto, m_textureSampler);

    CreateFrameBuffer();
}

void VulkanRenderer2D::CreateShaders(RenderSys::Shader& shader)
{
    assert(shader.type == RenderSys::ShaderType::SPIRV);
    std::vector<uint32_t> compiledShader;
    auto shaderMapIter = m_shaderMap.find(shader.GetName());
    if (shaderMapIter == m_shaderMap.end())
    {
        auto res = shader.Compile();
        assert(res);
        compiledShader = shader.GetCompiledShader();
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
        m_vertexBuffer = VK_NULL_HANDLE;
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    if (m_indexBuffer != VK_NULL_HANDLE && m_indexBufferMemory != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vma, m_indexBuffer, m_indexBufferMemory);
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < m_uniformBuffers.size(); i++)
    {
        vmaDestroyBuffer(m_vma, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
    }
    m_uniformBuffers.clear();
    m_uniformBuffersMemory.clear();
    m_uniformBuffersMapped.clear();
}

void VulkanRenderer2D::DestroyImages()
{
    // Called on every resize; the UI may still be sampling the old image. The first call has nothing to wait for.
    if (m_ImageToRenderInto != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(Vulkan::GetDevice());
    }

    // The framebuffer references the render target's view, so it goes first
    if (m_frameBuffer)
    {
        vkDestroyFramebuffer(Vulkan::GetDevice(), m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }

    if (m_finalRenderTarget)
    {
        Vulkan::DestroyRenderTarget(*m_finalRenderTarget);
    }
    m_finalRenderTarget.reset();

    if (m_textureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(Vulkan::GetDevice(), m_textureSampler, nullptr);
        m_textureSampler = VK_NULL_HANDLE;
    }

    if (m_ImageToRenderInto != VK_NULL_HANDLE)
    {
        vkDestroyImage(Vulkan::GetDevice(), m_ImageToRenderInto, nullptr);
        m_ImageToRenderInto = VK_NULL_HANDLE;
    }

    if (m_renderImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Vulkan::GetDevice(), m_renderImageMemory, nullptr);
        m_renderImageMemory = VK_NULL_HANDLE;
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

void VulkanRenderer2D::CreateBindGroup(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
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
        uboLayoutBinding.stageFlags = RenderSys::Vulkan::GetVulkanShaderStageVisibility(bindGroupLayoutEntry.visibility);
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

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_bindGroupPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_bindGroupLayout;

        if (vkAllocateDescriptorSets(Vulkan::GetDevice(), &allocInfo, &m_bindGroup) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }
}

bool VulkanRenderer2D::CreateRenderPass()
{
    if (m_renderpass)
        return true;

    VkAttachmentDescription colorAtt{};
    colorAtt.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // Matches the layout CreateRenderTarget() declares for the ImGui descriptor set that
    // samples this image (VulkanRendererUtils.cpp); GENERAL here was a validation-hidden mismatch.
    colorAtt.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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

    // Outgoing dependency: ImGui's fragment shader samples the color attachment for display after this pass.
    // Only this one is passed, so the incoming dependency stays the implicit one.
    VkSubpassDependency colorOutDep{};
    colorOutDep.srcSubpass = 0;
    colorOutDep.dstSubpass = VK_SUBPASS_EXTERNAL;
    colorOutDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    colorOutDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    colorOutDep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    colorOutDep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

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
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &colorOutDep;

    if (vkCreateRenderPass(Vulkan::GetDevice(), &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
    {
        std::cout << "error; could not create renderpass" << std::endl;
        return false;
    }

    return true;
}

void VulkanRenderer2D::CreatePipeline()
{
    // Examples without uniforms have no bind group, and a null set layout is invalid
    std::vector<VkDescriptorSetLayout> layouts;
    if (m_bindGroupLayout != VK_NULL_HANDLE)
        layouts.push_back(m_bindGroupLayout);
    m_render2DPipeline = std::make_unique<Vulkan::Render2DPipeline>(m_renderpass, layouts, m_vertexInputLayout, m_shaderStageInfos);
    if (m_render2DPipeline->GetPipeline() == VK_NULL_HANDLE || m_render2DPipeline->GetPipelineLayout() == VK_NULL_HANDLE){
        std::cout << "error: could not create pipeline" << std::endl;
        assert(false);
        return;
    }

    /* it is save to destroy the shader modules after pipeline has been created */
    DestroyShaders();
}

void VulkanRenderer2D::CreateFrameBuffer()
{
    if (m_frameBuffer)
    {
        vkDestroyFramebuffer(Vulkan::GetDevice(), m_frameBuffer, nullptr);
    }

    VkImageView frameBufferAttachments[] = { m_finalRenderTarget->view };
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
    if (m_vertexInputLayout.m_vertexAttribDescs.size() == 0)
    {
        m_vertexInputLayout.m_vertexBindingDescs.binding = 0;
        m_vertexInputLayout.m_vertexBindingDescs.stride = bufferLayout.arrayStride;
        m_vertexInputLayout.m_vertexBindingDescs.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        for (size_t i = 0; i < bufferLayout.attributeCount; i++)
        {
            RenderSys::VertexAttribute attrib = bufferLayout.attributes[i];
            VkVertexInputAttributeDescription vkAttribute{};
            vkAttribute.binding = 0;
            vkAttribute.location = attrib.location;
            vkAttribute.format = RenderSys::Vulkan::RenderSysFormatToVulkanFormat(attrib.format);
            vkAttribute.offset = attrib.offset;
            m_vertexInputLayout.m_vertexAttribDescs.push_back(vkAttribute);
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
    }

    std::cout << "Vertex buffer: " << m_vertexBuffer << std::endl;
}

void VulkanRenderer2D::CreateIndexBuffer(const std::vector<uint16_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;

    m_indexCount = bufferData.size();
    assert(m_indexCount > 0);
    
    // Per renderer: a static flag here left every renderer after the first one without an index buffer
    if (m_indexBuffer == VK_NULL_HANDLE)
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
    }

    std::cout << "Index buffer: " << m_indexBuffer << std::endl;
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
    bufferInfo.size = m_sizeOfOneUniform + ((maximumUniformIndex == 0) ? 0 : RenderSys::Vulkan::GetUniformStride(m_sizeOfOneUniform));
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

    auto offset = (uniformIndex == 0) ? 0 : RenderSys::Vulkan::GetUniformStride(m_sizeOfOneUniform);
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
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render2DPipeline->GetPipeline());

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

void VulkanRenderer2D::Render()
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render2DPipeline->GetPipeline());

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

    VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &m_vertexBuffer, &offset);
    vkCmdDraw(m_commandBuffer, m_vertexCount, 1, 0, 0);
}

void VulkanRenderer2D::RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount)
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render2DPipeline->GetPipeline());

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

    VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &m_vertexBuffer, &offset);
    vkCmdBindIndexBuffer(m_commandBuffer, m_indexBuffer, offset, VK_INDEX_TYPE_UINT16);

    if (m_bindGroup)
    {
        uint32_t dynamicOffset = (uniformIndex == 0) ? 0 : RenderSys::Vulkan::GetUniformStride(m_sizeOfOneUniform);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_render2DPipeline->GetPipelineLayout(),
                                     0, 1, &m_bindGroup, dynamicOffsetCount, &dynamicOffset);
        vkCmdDrawIndexed(m_commandBuffer, m_indexCount, 1, 0, 0, 0);
    }
    else
    {
        vkCmdDrawIndexed(m_commandBuffer, m_indexCount, 1, 0, 0, 0);
    }
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

uint64_t VulkanRenderer2D::GetDescriptorSet()
{
    return (uint64_t)m_finalRenderTarget->descriptorSet;
}

void VulkanRenderer2D::BeginRenderPass()
{
    VkClearValue colorClearValue;
    colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };
    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;
    VkClearValue clearValues[] = { colorClearValue, depthValue };

    // TODO: move commandpool/commandbuffer creation to constructor
    if (!m_commandPool)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = Vulkan::GetQueueFamily();
        auto err = vkCreateCommandPool(Vulkan::GetDevice(), &poolInfo, nullptr, &m_commandPool);
        Vulkan::CheckVkResult(err);
    }
    
    if (!m_commandBuffer)
    {
        VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = m_commandPool;
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufAllocateInfo.commandBufferCount = 1;
        auto err = vkAllocateCommandBuffers(Vulkan::GetDevice(), &cmdBufAllocateInfo, &m_commandBuffer);
        Vulkan::CheckVkResult(err);
    }
    else
    {
        auto err = vkResetCommandBuffer(m_commandBuffer, 0);
        Vulkan::CheckVkResult(err);
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    auto err = vkBeginCommandBuffer(m_commandBuffer, &begin_info);
    Vulkan::CheckVkResult(err);

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

void VulkanRenderer2D::EndRenderPass()
{
	vkCmdEndRenderPass(m_commandBuffer);
    SubmitCommandBuffer();
}

void VulkanRenderer2D::Destroy()
{
    // Also called by the destructor, so it may run twice
    if (!m_vma)
        return;

    // DestroyImages waits for the device, so nothing below is still in use
    DestroyImages();
    DestroyBuffers();
    DestroyShaders();
    m_render2DPipeline.reset();

    if (m_bindGroupPool != VK_NULL_HANDLE)
    {
        // Frees m_bindGroup with it
        vkDestroyDescriptorPool(Vulkan::GetDevice(), m_bindGroupPool, nullptr);
        m_bindGroupPool = VK_NULL_HANDLE;
        m_bindGroup = VK_NULL_HANDLE;
    }
    if (m_bindGroupLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(Vulkan::GetDevice(), m_bindGroupLayout, nullptr);
        m_bindGroupLayout = VK_NULL_HANDLE;
    }
    m_bindGroupBindings.clear();

    if (m_commandPool != VK_NULL_HANDLE)
    {
        // Frees m_commandBuffer with it
        vkDestroyCommandPool(Vulkan::GetDevice(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
        m_commandBuffer = VK_NULL_HANDLE;
    }
    if (m_renderpass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(Vulkan::GetDevice(), m_renderpass, nullptr);
        m_renderpass = VK_NULL_HANDLE;
    }

    vmaDestroyAllocator(m_vma);
    m_vma = VK_NULL_HANDLE;
}

void VulkanRenderer2D::SubmitCommandBuffer()
{
    auto err = vkEndCommandBuffer(m_commandBuffer);
    Vulkan::CheckVkResult(err);

    VkSubmitInfo end_info{};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &m_commandBuffer;
    Vulkan::QueueSubmitAndWait(end_info);
}

} // namespace GraphicsAPI