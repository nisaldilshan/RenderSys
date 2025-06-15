#include "VulkanRenderer3D.h"

#include "VulkanRendererUtils.h"
#include "VulkanMemAlloc.h"
#include "VulkanTexture.h"
#include "VulkanMaterial.h"
#include "VulkanResource.h"
#include "VulkanShadowMap.h"
#include "Pipeline/VulkanPbrRenderPipeline.h"
#include "Pipeline/VulkanShadowRenderPipeline.h"

#include <array>
#include <iostream>

namespace RenderSys
{

VulkanRenderer3D::VulkanRenderer3D()
{
}

VulkanRenderer3D::~VulkanRenderer3D()
{
}

bool VulkanRenderer3D::Init()
{
    CreateRenderPass();
    CreateCommandBuffers();
    CreateDefaultTextureSampler();
    RenderSys::CreateMaterialBindGroupPool();
    RenderSys::CreateMaterialBindGroupLayout();
    RenderSys::CreateResourceBindGroupPool();
    RenderSys::CreateResourceBindGroupLayout();
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

    if (vmaCreateImage(RenderSys::Vulkan::GetMemoryAllocator(), &renderImageInfo, &allocInfo, &m_ImageToRenderInto, &m_renderImageMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }

    m_imageViewToRenderInto = RenderSys::Vulkan::CreateImageView(m_ImageToRenderInto, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    m_finalImageDescriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(m_defaultTextureSampler, m_imageViewToRenderInto, VK_IMAGE_LAYOUT_GENERAL);
}

void VulkanRenderer3D::CreateDepthImage()
{
    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = Vulkan::GetDepthFormat();
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

    if (vmaCreateImage(RenderSys::Vulkan::GetMemoryAllocator(), &depthImageInfo, &allocInfo, &m_depthimage, &m_depthimageMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
    }

    m_depthimageView = RenderSys::Vulkan::CreateImageView(m_depthimage, Vulkan::GetDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanRenderer3D::CreateShaders(RenderSys::Shader& shader)
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
    if (vkCreateShaderModule(GraphicsAPI::Vulkan::GetDevice(), &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
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
    if (m_finalImageDescriptorSet)
    {
        vkQueueWaitIdle(GraphicsAPI::Vulkan::GetDeviceQueue());
        ImGui_ImplVulkan_RemoveTexture(m_finalImageDescriptorSet);
        m_finalImageDescriptorSet = VK_NULL_HANDLE;
    }

    if (m_frameBuffer)
    {
        vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
        vkDestroyFramebuffer(GraphicsAPI::Vulkan::GetDevice(), m_frameBuffer, nullptr);
        m_frameBuffer = VK_NULL_HANDLE;
    }
    
    if (m_imageViewToRenderInto != VK_NULL_HANDLE)
    {
        vkDestroyImageView(GraphicsAPI::Vulkan::GetDevice(), m_imageViewToRenderInto, nullptr);
        m_imageViewToRenderInto = VK_NULL_HANDLE;
    }
    if (m_ImageToRenderInto != VK_NULL_HANDLE)
    {
        vmaDestroyImage(RenderSys::Vulkan::GetMemoryAllocator(), m_ImageToRenderInto, m_renderImageMemory);
        m_ImageToRenderInto = VK_NULL_HANDLE;
    }

    if (m_depthimageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(GraphicsAPI::Vulkan::GetDevice(), m_depthimageView, nullptr);
        m_depthimageView = VK_NULL_HANDLE;
    }
    if (m_depthimage != VK_NULL_HANDLE)
    {
        vmaDestroyImage(RenderSys::Vulkan::GetMemoryAllocator(), m_depthimage, m_depthimageMemory);
        m_depthimage = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::DestroyBindGroup()
{
    if (m_mainBindGroup && m_bindGroupPool)
    {
        vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
        // when you destroy a descriptor pool, all descriptor sets allocated from that pool are automatically destroyed
        vkDestroyDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), m_bindGroupPool, nullptr);
        m_mainBindGroup = VK_NULL_HANDLE;
        m_bindGroupPool = VK_NULL_HANDLE;
    }

    if (m_bindGroupLayout)
    {
        vkDestroyDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), m_bindGroupLayout, nullptr);
        m_bindGroupLayout = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::DestroyBuffers()
{
    for (auto [id, vertexIndexBufferInfo] : m_vertexIndexBufferInfoMap)
    {
        if (vertexIndexBufferInfo->m_vertexBuffer != VK_NULL_HANDLE && vertexIndexBufferInfo->m_vertexBufferMemory != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_vertexBuffer, vertexIndexBufferInfo->m_vertexBufferMemory);
            vertexIndexBufferInfo->m_vertexBuffer = VK_NULL_HANDLE;
            vertexIndexBufferInfo->m_vertexBufferMemory = VK_NULL_HANDLE;
        }
    
        if (vertexIndexBufferInfo->m_indexBuffer != VK_NULL_HANDLE && vertexIndexBufferInfo->m_indexBufferMemory != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_indexBuffer, vertexIndexBufferInfo->m_indexBufferMemory);
            vertexIndexBufferInfo->m_indexBuffer = VK_NULL_HANDLE;
            vertexIndexBufferInfo->m_indexBufferMemory = VK_NULL_HANDLE;
        }
    }
    m_vertexIndexBufferInfoMap.clear();

    for (auto& [_ , uniformBufferTuple] : m_uniformBuffers)
    {
        VkDescriptorBufferInfo& bufferInfo = std::get<0>(uniformBufferTuple);
        VmaAllocation& uniformBufferMemory = std::get<1>(uniformBufferTuple);
        vmaDestroyBuffer(RenderSys::Vulkan::GetMemoryAllocator(), bufferInfo.buffer, uniformBufferMemory);
    }
    m_uniformBuffers.clear();

    if (RenderSys::Vulkan::GetCommandPool())
    {
        // when you destroy a command pool, all command buffers allocated from that pool are automatically destroyed
        RenderSys::Vulkan::DestroyCommandPool();
    }   
    m_commandBuffer = VK_NULL_HANDLE; 
}

void VulkanRenderer3D::DestroyShaders()
{
    for (auto& shaderStageInfo : m_shaderStageInfos)
    {
        vkDestroyShaderModule(GraphicsAPI::Vulkan::GetDevice(), shaderStageInfo.module, nullptr);
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
        auto vkBinding = RenderSys::Vulkan::GetVulkanBindGroupLayoutEntry(bindGroupLayoutEntry);
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

    if (vkCreateDescriptorSetLayout(GraphicsAPI::Vulkan::GetDevice(), &layoutInfo, nullptr, &m_bindGroupLayout) != VK_SUCCESS) {
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

    if (vkCreateDescriptorPool(GraphicsAPI::Vulkan::GetDevice(), &poolInfo, nullptr, &m_bindGroupPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_bindGroupPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_bindGroupLayout;

    if (vkAllocateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), &allocInfo, &m_mainBindGroup) != VK_SUCCESS) {
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
        VkDescriptorImageInfo imageInfo{};
        if (textureIter != m_textures.end())
        {
            imageInfo = *textureIter->second->GetDescriptorImageInfoAddr();
            if (imageInfo.sampler == VK_NULL_HANDLE)
            {
                imageInfo.sampler = m_defaultTextureSampler;
            }
            descriptorWrite.pImageInfo = &imageInfo;
        }

        if (descriptorWrite.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            assert(false);
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = m_defaultTextureSampler;
            descriptorWrite.pImageInfo = &imageInfo;
        }
        
        vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
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
    depthAtt.format = Vulkan::GetDepthFormat();
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

    if (vkCreateRenderPass(GraphicsAPI::Vulkan::GetDevice(), &renderPassInfo, nullptr, &m_renderpass) != VK_SUCCESS)
    {
        std::cout << "error; could not create renderpass" << std::endl;
        assert(false);
    }
}

void VulkanRenderer3D::CreateCommandBuffers()
{
    RenderSys::Vulkan::CreateCommandPool();

    VkCommandBufferAllocateInfo cmdBufAllocateInfo{};
    cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool = RenderSys::Vulkan::GetCommandPool();
    cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount = 1;
    auto err = vkAllocateCommandBuffers(GraphicsAPI::Vulkan::GetDevice(), &cmdBufAllocateInfo, &m_commandBuffer);
    GraphicsAPI::Vulkan::check_vk_result(err);
}

void VulkanRenderer3D::DestroyRenderPass()
{
    vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
    vkDestroyRenderPass(GraphicsAPI::Vulkan::GetDevice(), m_renderpass, nullptr);
}

void VulkanRenderer3D::CreatePipeline()
{
    std::vector<VkDescriptorSetLayout> layouts{m_bindGroupLayout, 
                                                    RenderSys::GetMaterialBindGroupLayout(),
                                                    RenderSys::GetResourceBindGroupLayout()};
    m_pbrRenderPipeline = std::make_unique<RenderSys::Vulkan::PbrRenderPipeline>(m_renderpass, layouts, m_vertexInputLayout, m_shaderStageInfos);
    if (m_pbrRenderPipeline->GetPipeline() == VK_NULL_HANDLE || m_pbrRenderPipeline->GetPipelineLayout() == VK_NULL_HANDLE){
        std::cout << "error: could not create pipeline" << std::endl;
        assert(false);
        return;
    }
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

    if (vkCreateFramebuffer(GraphicsAPI::Vulkan::GetDevice(), &FboInfo, nullptr, &m_frameBuffer) != VK_SUCCESS) {
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
    }
    
    auto vertexIndexBufferInfo = std::make_shared<Vulkan::VertexIndexBufferInfo>();
    vertexIndexBufferInfo->m_vertexCount = vertexCount;
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferLength;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), &bufferInfo, &vmaAllocInfo, 
                        &vertexIndexBufferInfo->m_vertexBuffer, &vertexIndexBufferInfo->m_vertexBufferMemory, nullptr) != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return 0;
    }

    // copy data to the buffer
    void *buf;
    auto res = vmaMapMemory(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_vertexBufferMemory, &buf);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return 0;
    }

    std::memcpy(buf, bufferData.vertices.data(), bufferLength);
    vmaUnmapMemory(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_vertexBufferMemory);

    std::cout << "Vertex buffer: " << vertexIndexBufferInfo->m_vertexBuffer << std::endl;
    const uint32_t key = m_vertexIndexBufferInfoMap.size() + 1;
    auto res2 = m_vertexIndexBufferInfoMap.insert({key, vertexIndexBufferInfo});
    return res2.first->first;
}

void VulkanRenderer3D::CreateIndexBuffer(uint32_t vertexBufferID, const std::vector<uint32_t> &bufferData)
{
    std::cout << "Creating index buffer..." << std::endl;
    assert(bufferData.size() > 0);
    assert(sizeof(bufferData[0]) == 4); // because we are using type - VK_INDEX_TYPE_UINT32

    const auto& vertexIndexBufferInfoIter = m_vertexIndexBufferInfoMap.find(vertexBufferID);
    if (vertexIndexBufferInfoIter == m_vertexIndexBufferInfoMap.end())
    {
        std::cout << "Error: could not find vertexIndexBufferInfo!" << std::endl;
        assert(false);
        return;
    }
    auto vertexIndexBufferInfo = vertexIndexBufferInfoIter->second;
    vertexIndexBufferInfo->m_indexCount = bufferData.size();
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = vertexIndexBufferInfo->m_indexCount * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    auto res = vmaCreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), &bufferInfo, &vmaAllocInfo, &vertexIndexBufferInfo->m_indexBuffer, &vertexIndexBufferInfo->m_indexBufferMemory, nullptr);
    if (res != VK_SUCCESS) {
        std::cout << "vkCreateBuffer() failed!" << std::endl;
        return;
    }

    // copy data to the buffer
    void *buf;
    res = vmaMapMemory(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_indexBufferMemory, &buf);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    std::memcpy(buf, bufferData.data(), bufferInfo.size);
    vmaUnmapMemory(RenderSys::Vulkan::GetMemoryAllocator(), vertexIndexBufferInfo->m_indexBufferMemory);
    std::cout << "Index buffer: " << vertexIndexBufferInfo->m_indexBuffer << std::endl;
}

void VulkanRenderer3D::SetClearColor(glm::vec4 clearColor)
{
    m_clearColor = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};
}

void VulkanRenderer3D::CreateUniformBuffer(uint32_t binding, uint32_t sizeOfOneUniform)
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

    auto res = vmaCreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), &bufferCreateInfo, &vmaAllocInfo, &bufferInfo.buffer, &uniformBufferMemory, nullptr);
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

    auto res = vmaMapMemory(RenderSys::Vulkan::GetMemoryAllocator(), uniformBufferMemory, &mappedBuffer);
    if (res != VK_SUCCESS) {
        std::cout << "vkMapMemory() failed" << std::endl;
        return;
    }

    // copy data to the buffer
    auto* ptr = static_cast<char*>(mappedBuffer);
    memcpy(static_cast<void*>(ptr), bufferData, sizeOfOneUniform);

    vmaUnmapMemory(RenderSys::Vulkan::GetMemoryAllocator(), uniformBufferMemory);

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
    

    vkUpdateDescriptorSets(GraphicsAPI::Vulkan::GetDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanRenderer3D::BindResources()
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pbrRenderPipeline->GetPipeline());
}

void VulkanRenderer3D::Render()
{
    RenderIndexed();
}

void VulkanRenderer3D::RenderIndexed()
{
    if (m_mainBindGroup)
    {
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pbrRenderPipeline->GetPipelineLayout(), 0
                                    , 1, &m_mainBindGroup, 0, nullptr);
    }

    assert(m_vertexIndexBufferInfoMap.size() == 1);
    for (auto [id, vertexIndexBufferInfo] : m_vertexIndexBufferInfoMap)
    {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vertexIndexBufferInfo->m_vertexBuffer, &offset);
        if (vertexIndexBufferInfo->m_indexCount > 0)
        {
            assert(vertexIndexBufferInfo->m_indexBuffer != VK_NULL_HANDLE);
            vkCmdBindIndexBuffer(m_commandBuffer, vertexIndexBufferInfo->m_indexBuffer, offset, VK_INDEX_TYPE_UINT32);
        }
        if (vertexIndexBufferInfo->m_indexCount > 0)
        {
            vkCmdDrawIndexed(m_commandBuffer, vertexIndexBufferInfo->m_indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(m_commandBuffer, vertexIndexBufferInfo->m_vertexCount, 1, 0, 0);
        }
    }
}

void VulkanRenderer3D::RenderMesh(const RenderSys::Mesh& mesh)
{
    auto vertexIndexBufferInfoIter = m_vertexIndexBufferInfoMap.find(mesh.vertexBufferID);
    assert(vertexIndexBufferInfoIter != m_vertexIndexBufferInfoMap.end());
    const auto& vertexIndexBufferInfo = vertexIndexBufferInfoIter->second;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &vertexIndexBufferInfo->m_vertexBuffer, &offset);
    if (vertexIndexBufferInfo->m_indexCount > 0)
    {
        assert(vertexIndexBufferInfo->m_indexBuffer != VK_NULL_HANDLE);
        vkCmdBindIndexBuffer(m_commandBuffer, vertexIndexBufferInfo->m_indexBuffer, offset, VK_INDEX_TYPE_UINT32);
    }

    assert(m_mainBindGroup != VK_NULL_HANDLE);
    for (const auto &subMesh : mesh.subMeshes)
    {
        assert(subMesh.m_Material);
        RenderSubMesh(mesh.vertexBufferID, subMesh);
    }
}

void VulkanRenderer3D::RenderSubMesh(const uint32_t vertexBufferID, const RenderSys::SubMesh& subMesh)
{
    auto materialBindGroup = subMesh.m_Material->GetDescriptor()->GetPlatformDescriptor()->m_bindGroup;
    assert(materialBindGroup != VK_NULL_HANDLE);
    auto resourceBindGroup = subMesh.m_Resource->GetDescriptor()->GetPlatformDescriptor()->m_bindGroup;
    assert(resourceBindGroup != VK_NULL_HANDLE);
    std::vector<VkDescriptorSet> descriptorsets{m_mainBindGroup, materialBindGroup, resourceBindGroup};

    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pbrRenderPipeline->GetPipelineLayout(), 0
                                , descriptorsets.size(), descriptorsets.data()
                                , 0, nullptr);

    vkCmdPushConstants(m_commandBuffer, m_pbrRenderPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, 
                    sizeof(RenderSys::MaterialProperties), &subMesh.m_Material->GetMaterialProperties());

    assert(vertexBufferID >= 1);
    const auto& vertexIndexBufferInfoIter = m_vertexIndexBufferInfoMap.find(vertexBufferID);
    if (vertexIndexBufferInfoIter == m_vertexIndexBufferInfoMap.end())
    {
        std::cout << "Error: could not find vertexIndexBufferInfo!" << std::endl;
        assert(false);
        return;
    }

    auto vertexIndexBufferInfo = vertexIndexBufferInfoIter->second;
    if (vertexIndexBufferInfo->m_indexCount > 0)
    {
        assert(subMesh.m_InstanceCount > 0);
        if (subMesh.m_IndexCount > 0)
        {
            vkCmdDrawIndexed(m_commandBuffer, subMesh.m_IndexCount, subMesh.m_InstanceCount, subMesh.m_FirstIndex, 0, 0);
        }
        else
        {
            vkCmdDrawIndexed(m_commandBuffer, vertexIndexBufferInfo->m_indexCount, subMesh.m_InstanceCount, 0, 0, 0);
        }
    }
    else
    {
        vkCmdDraw(m_commandBuffer, vertexIndexBufferInfo->m_vertexCount, 1, 0, 0);
    }
}

void VulkanRenderer3D::DrawPlane()
{
}

void VulkanRenderer3D::DrawCube()
{
}

ImTextureID VulkanRenderer3D::GetDescriptorSet()
{
    return m_finalImageDescriptorSet;
}

void VulkanRenderer3D::BeginRenderPass()
{
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = m_clearColor;
    clearValues[1].depthStencil = {1.0f, 0};

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
}

void VulkanRenderer3D::EndRenderPass()
{
    vkCmdEndRenderPass(m_commandBuffer);
}

void VulkanRenderer3D::BeginShadowMapPass()
{
    if (!m_shadowMap)
    {
        m_shadowMap = std::make_shared<RenderSys::Vulkan::ShadowMap>(10);
    }

    if (!m_commandBuffer)
        return;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_shadowMap->GetShadowRenderPass();
    renderPassInfo.framebuffer = m_shadowMap->GetShadowFrameBuffer();

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_shadowMap->GetShadowMapExtent();

    std::array<VkClearValue, static_cast<uint32_t>(Vulkan::ShadowMap::ShadowRenderTargets::NUMBER_OF_ATTACHMENTS)> clearValues{};
    clearValues[0].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_shadowMap->GetShadowMapExtent().width);
    viewport.height = static_cast<float>(m_shadowMap->GetShadowMapExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{{0, 0}, m_shadowMap->GetShadowMapExtent()};
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
}

void VulkanRenderer3D::RenderShadowMap()
{
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pbrRenderPipeline->GetPipeline());

    // auto meshView = registry.Get().view<MeshComponent, TransformComponent, InstanceTag>(
    //     entt::exclude<SkeletalAnimationTag, GrassTag, Grass2Tag>);
    // for (auto entity : meshView)
    // {
    //     auto& mesh = meshView.get<MeshComponent>(entity);
    //     if (mesh.m_Enabled)
    //     {
    //         static_cast<VK_Model*>(mesh.m_Model.get())->Bind(frameInfo.m_CommandBuffer);
    //         static_cast<VK_Model*>(mesh.m_Model.get())
    //             ->DrawShadowInstanced(frameInfo, m_PipelineLayout, shadowDescriptorSet);
    //     }
    // }
}

void VulkanRenderer3D::EndShadowMapPass()
{
    if (!m_commandBuffer)
        return;

    vkCmdEndRenderPass(m_commandBuffer);
}

void VulkanRenderer3D::DestroyTextures()
{
    m_textures.clear();
    
    if (m_defaultTextureSampler)
    {
        vkDestroySampler(GraphicsAPI::Vulkan::GetDevice(), m_defaultTextureSampler, nullptr);
        m_defaultTextureSampler = VK_NULL_HANDLE;
    }
}

void VulkanRenderer3D::Destroy()
{
    RenderSys::DestroyResourceBindGroupLayout();
    RenderSys::DestroyResourceBindGroupPool();
    RenderSys::DestroyMaterialBindGroupLayout();
    RenderSys::DestroyMaterialBindGroupPool();

    DestroyShaders();
    DestroyBindGroup();
    DestroyBuffers();
    DestroyImages();
    DestroyTextures();
    DestroyRenderPass();

    RenderSys::Vulkan::DestroyMemoryAllocator();
}

void VulkanRenderer3D::ResetCommandBuffer()
{
    auto err = vkResetCommandBuffer(m_commandBuffer, 0);
    GraphicsAPI::Vulkan::check_vk_result(err);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(m_commandBuffer, &begin_info);
    GraphicsAPI::Vulkan::check_vk_result(err);
}

void VulkanRenderer3D::SubmitCommandBuffer()
{
    auto err = vkEndCommandBuffer(m_commandBuffer);
    GraphicsAPI::Vulkan::check_vk_result(err);

    VkSubmitInfo end_info{};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &m_commandBuffer;
    GraphicsAPI::Vulkan::QueueSubmit(end_info);
}

void VulkanRenderer3D::CreateTexture(uint32_t binding, const std::shared_ptr<RenderSys::Texture> texture)
{
    const auto& [textureIter, inserted] = m_textures.insert(
                                                    {
                                                        binding, 
                                                        texture->GetPlatformTexture()
                                                    }
                                                );
    if (!inserted)
    {
        return;
    }
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

    if (vkCreateSampler(GraphicsAPI::Vulkan::GetDevice(), &texSamplerInfo, nullptr, &m_defaultTextureSampler) != VK_SUCCESS) {
        std::cout << "error: could not create sampler for texture" << std::endl;
    }
}

} // namespace GraphicsAPI