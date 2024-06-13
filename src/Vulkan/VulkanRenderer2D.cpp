#include "VulkanRenderer2D.h"

#include <iostream>
#include "vkb/VkBootstrap.h"
#include "vma/vk_mem_alloc.h"

namespace GraphicsAPI
{

uint32_t g_acquiredFrameIndex = 0;
vkb::Instance g_vkbInstance;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;
vkb::PhysicalDevice g_vkbPhysDevice;
vkb::Device g_vkbDevice;
vkb::Swapchain g_vkbSwapChain;

VmaAllocator g_allocator;

VkCommandPool g_commandPool = VK_NULL_HANDLE;
VkCommandBuffer g_commandBuffer = VK_NULL_HANDLE;

VkSemaphore g_presentSemaphore = VK_NULL_HANDLE;
VkSemaphore g_renderSemaphore = VK_NULL_HANDLE;
VkFence g_renderFence = VK_NULL_HANDLE;

std::vector<VkImage> g_rdSwapchainImages;
std::vector<VkImageView> g_rdSwapchainImageViews;
std::vector<VkFramebuffer> g_framebuffers;

VkImage g_depthImage = VK_NULL_HANDLE;
VkImageView g_depthImageView = VK_NULL_HANDLE;
constexpr VkFormat g_rdDepthFormat = VK_FORMAT_D32_SFLOAT;
VmaAllocation g_depthImageAllocation = VK_NULL_HANDLE;

VkRenderPass g_renderpass = VK_NULL_HANDLE;
VkPipelineLayout g_pipelineLayout = VK_NULL_HANDLE;
VkPipeline g_pipeline = VK_NULL_HANDLE;

int g_triangleCount = 0;
VkBuffer g_vertexBuffer;
VmaAllocation g_vertexBufferAllocation;

bool deviceInit()
{
    /* instance and window */
    vkb::InstanceBuilder instBuild;
    auto instRet = instBuild.use_default_debug_messenger().request_validation_layers().build();
    // auto instRet = instBuild.build();
    if (!instRet)
    {
        std::cout << "error: could not build vkb instance" << std::endl;
        return false;
    }
    g_vkbInstance = instRet.value();

    VkResult result = VK_ERROR_UNKNOWN;
    result = glfwCreateWindowSurface(g_vkbInstance, Vulkan::GetWindowHandle(), nullptr, &g_surface);
    if (result != VK_SUCCESS)
    {
        std::cout << "error: Could not create Vulkan surface" << std::endl;
        return false;
    }

    /* just get the first available device */
    vkb::PhysicalDeviceSelector physicalDevSel{g_vkbInstance};
    auto physicalDevSelRet = physicalDevSel.set_surface(g_surface).select();
    if (!physicalDevSelRet)
    {
        std::cout << "error: could not get physical devices" << std::endl;
        return false;
    }
    g_vkbPhysDevice = physicalDevSelRet.value();
    std::cout << "found physical device : " << g_vkbPhysDevice.name.c_str() << std::endl;

    vkb::DeviceBuilder devBuilder{g_vkbPhysDevice};
    auto devBuilderRet = devBuilder.build();
    if (!devBuilderRet)
    {
        std::cout << "error: could not get devices" << std::endl;
        return false;
    }
    g_vkbDevice = devBuilderRet.value();

    return true;
}

bool createCommandBuffer()
{
    VkCommandPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.queueFamilyIndex = g_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(g_vkbDevice.device, &poolCreateInfo, nullptr, &g_commandPool) != VK_SUCCESS) {
        std::cout << "error: could not create command pool" << std::endl;
        return false;
    }

    VkCommandBufferAllocateInfo bufferAllocInfo{};
    bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    bufferAllocInfo.commandPool = g_commandPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(g_vkbDevice.device, &bufferAllocInfo, &g_commandBuffer) != VK_SUCCESS) {
        std::cout << "error: could not allocate command buffers" << std::endl;
        return false;
    }

    return true;

    // call vkDestroyCommandPool(renderData.rdVkbDevice.device, renderData.rdCommandPool, nullptr); to destroy commandpool
    // call vkFreeCommandBuffers(renderData.rdVkbDevice.device, renderData.rdCommandPool, 1, &commandBuffer); to destroy command buffer
}

bool createRenderPass() 
{
  VkAttachmentDescription colorAtt{};
  colorAtt.format = g_vkbSwapChain.image_format;
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttRef{};
  colorAttRef.attachment = 0;
  colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAtt{};
  depthAtt.flags = 0;
  depthAtt.format = g_rdDepthFormat;
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

  VkSubpassDependency dependencies[] = { subpassDep, depthDep };
  VkAttachmentDescription attachments[] = { colorAtt, depthAtt };

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 2;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = 2;
  renderPassInfo.pDependencies = dependencies;

  if (vkCreateRenderPass(g_vkbDevice.device, &renderPassInfo, nullptr, &g_renderpass) != VK_SUCCESS) {
    std::cout<< "error; could not create renderpass" << std::endl;
    return false;
  }

  return true;
}


bool initVma()
{
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = g_vkbPhysDevice.physical_device;
    allocatorInfo.device = g_vkbDevice.device;
    allocatorInfo.instance = g_vkbInstance.instance;
    if (vmaCreateAllocator(&allocatorInfo, &g_allocator) != VK_SUCCESS) {
        std::cout << "error: could not init VMA" << std::endl;
        return false;
    }

    return true;
}

bool createDepthBuffer() 
{
    VkExtent3D depthImageExtent = { g_vkbSwapChain.extent.width, g_vkbSwapChain.extent.height, 1 };

    VkImageCreateInfo depthImageInfo{};
    depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageInfo.format = g_rdDepthFormat;
    depthImageInfo.extent = depthImageExtent;
    depthImageInfo.mipLevels = 1;
    depthImageInfo.arrayLayers = 1;
    depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VmaAllocationCreateInfo depthAllocInfo{};
    depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depthAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vmaCreateImage(g_allocator, &depthImageInfo, &depthAllocInfo, &g_depthImage, &g_depthImageAllocation, nullptr) != VK_SUCCESS) {
        std::cout << "error: could not allocate depth buffer memory" << std::endl;
        return false;
    }

    VkImageViewCreateInfo depthImageViewinfo{};
    depthImageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewinfo.image = g_depthImage;
    depthImageViewinfo.format = g_rdDepthFormat;
    depthImageViewinfo.subresourceRange.baseMipLevel = 0;
    depthImageViewinfo.subresourceRange.levelCount = 1;
    depthImageViewinfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewinfo.subresourceRange.layerCount = 1;
    depthImageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (vkCreateImageView(g_vkbDevice.device, &depthImageViewinfo, nullptr, &g_depthImageView) != VK_SUCCESS) {
        std::cout << "error: could not create depth buffer image view" << std::endl;
        return false;
    }
    return true;
}

bool createFrameBuffers()
{
    g_rdSwapchainImages = g_vkbSwapChain.get_images().value();
    g_rdSwapchainImageViews = g_vkbSwapChain.get_image_views().value();

    g_framebuffers.resize(g_rdSwapchainImageViews.size());

    for (unsigned int i = 0; i < g_rdSwapchainImageViews.size(); ++i) {
        VkImageView attachments[] = { g_rdSwapchainImageViews.at(i), g_depthImageView };

        VkFramebufferCreateInfo FboInfo{};
        FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FboInfo.renderPass = g_renderpass;
        FboInfo.attachmentCount = 2;
        FboInfo.pAttachments = attachments;
        FboInfo.width = g_vkbSwapChain.extent.width;
        FboInfo.height = g_vkbSwapChain.extent.height;
        FboInfo.layers = 1;

        if (vkCreateFramebuffer(g_vkbDevice.device, &FboInfo, nullptr, &g_framebuffers[i]) != VK_SUCCESS) {
            std::cout << "error: failed to create framebuffer" << std::endl;
            return false;
        }
    }
    return true;

    // do destroy
    // for (auto &fb : renderData.rdFramebuffers) {
    //     vkDestroyFramebuffer(renderData.rdVkbDevice.device, fb, nullptr);
    // }
}

bool createSyncObjects()
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(g_vkbDevice.device, &semaphoreInfo, nullptr, &g_presentSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(g_vkbDevice.device, &semaphoreInfo, nullptr, &g_renderSemaphore) != VK_SUCCESS ||
        vkCreateFence(g_vkbDevice.device, &fenceInfo, nullptr, &g_renderFence) != VK_SUCCESS) {
        std::cout << "error: failed to init sync objects" << std::endl;
        return false;
    }
    return true;
}

bool VulkanRenderer2D::Init()
{
    if (!deviceInit()) {
        return false;
    }

    if (!initVma()) {
        return false;
    }

    // if (!getQueue()) {
    //     return false;
    // }

    if (!CreateSwapChain()) {
        return false;
    }

    if (!createDepthBuffer()) {
        return false;
    }

    if (!createRenderPass())
    {
        return false;
    }

    CreatePipeline();

    if (!createFrameBuffers())
    {
        return false;
    }

    if (!createSyncObjects()) {
        return false;
    }


    return true;
}

void VulkanRenderer2D::CreateTextureToRenderInto(uint32_t width, uint32_t height)
{
    // m_width = width;
    // m_height = height;
    // wgpu::TextureDescriptor tex_desc = {};
    // tex_desc.label = "Renderer Final Texture";
    // tex_desc.dimension = WGPUTextureDimension_2D;
    // tex_desc.size.width = m_width;
    // tex_desc.size.height = m_height;
    // tex_desc.size.depthOrArrayLayers = 1;
    // tex_desc.sampleCount = 1;
    // tex_desc.format = WGPUTextureFormat_BGRA8Unorm;
    // tex_desc.mipLevelCount = 1;
    // tex_desc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding | WGPUTextureUsage_RenderAttachment;
    // //##
    // tex_desc.viewFormatCount = 1;
    // wgpu::TextureFormat tf = WebGPU::GetSwapChainFormat();
	// tex_desc.viewFormats = (WGPUTextureFormat *)const_cast<wgpu::TextureFormat *>(&tf);
    // //##
    // wgpu::Texture texture = WebGPU::GetDevice().createTexture(tex_desc);

    // wgpu::TextureViewDescriptor tex_view_desc = {};
    // tex_view_desc.format = WGPUTextureFormat_BGRA8Unorm;
    // tex_view_desc.dimension = WGPUTextureViewDimension_2D;
    // tex_view_desc.baseMipLevel = 0;
    // tex_view_desc.mipLevelCount = 1;
    // tex_view_desc.baseArrayLayer = 0;
    // tex_view_desc.arrayLayerCount = 1;
    // tex_view_desc.aspect = WGPUTextureAspect_All;
    // m_textureToRenderInto = texture.createView(tex_view_desc);
}

void VulkanRenderer2D::CreateShaders(std::string shaderSource)
{
    std::cout << "Creating shader module..." << std::endl;

    VkShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.codeSize = shaderSource.size();
    /* casting needed to align the code to 32 bit */
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSource.c_str());

    if (vkCreateShaderModule(g_vkbDevice.device, &shaderCreateInfo, nullptr, &m_shaderModule) != VK_SUCCESS) {
        std::cout << "could not load shader" << std::endl;
        return;
    }

    std::cout << "Shader module: " << m_shaderModule << std::endl;
}

void VulkanRenderer2D::CreateStandaloneShader(const char *shaderSource, uint32_t vertexShaderCallCount)
{
    // CreateShaders(shaderSource);
    // m_vertexCount = vertexShaderCallCount;
}

void VulkanRenderer2D::SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    // Create a bind group layout
    m_bindGroupLayout = std::make_unique<VkPipelineLayoutCreateInfo>();
    m_bindGroupLayout->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    m_bindGroupLayout->setLayoutCount = 0; // was 1
    //m_bindGroupLayout->pSetLayouts = &renderData.rdTextureLayout;
    m_bindGroupLayout->pushConstantRangeCount = 0;
}

void VulkanRenderer2D::CreateBindGroup()
{
    if (m_bindGroupLayout)
    {
        if (vkCreatePipelineLayout(g_vkbDevice.device, m_bindGroupLayout.get(), nullptr, &g_pipelineLayout) != VK_SUCCESS) {
            std::cout << "error: could not create pipeline layout" << std::endl;
        }
    }
    else
    {
        std::cout << "No bind group layout" << std::endl;
    }
}

void VulkanRenderer2D::CreatePipeline()
{
    std::cout << "Creating render pipeline..." << std::endl;

    if (m_shaderModule == VK_NULL_HANDLE) {
        std::cout << "error: could not load shaders" << std::endl;
        return;
    }

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = m_shaderModule;
    vertexStageInfo.pName = "vs_main";

    VkPipelineShaderStageCreateInfo fragmentStageInfo{};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = m_shaderModule;
    fragmentStageInfo.pName = "fs_main";

    VkPipelineShaderStageCreateInfo shaderStagesInfo[] = { vertexStageInfo, fragmentStageInfo };

    /* assemble the graphics pipeline itself */
    VkVertexInputBindingDescription mainBinding{};
    mainBinding.binding = 0;
    mainBinding.stride = sizeof(RenderSysVkVertex);
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription positionAttribute{};
    positionAttribute.binding = 0;
    positionAttribute.location = 0;
    positionAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    positionAttribute.offset = offsetof(RenderSysVkVertex, position);

    VkVertexInputAttributeDescription uvAttribute{};
    uvAttribute.binding = 0;
    uvAttribute.location = 1;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(RenderSysVkVertex, uv);

    VkVertexInputAttributeDescription attributes[] = { positionAttribute, uvAttribute };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &mainBinding;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributes;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(g_vkbSwapChain.extent.width);
    viewport.height = static_cast<float>(g_vkbSwapChain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = g_vkbSwapChain.extent;

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

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStagesInfo;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilInfo;
    pipelineCreateInfo.pDynamicState = &dynStatesInfo;
    pipelineCreateInfo.layout = g_pipelineLayout;
    pipelineCreateInfo.renderPass = g_renderpass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(g_vkbDevice.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &g_pipeline) != VK_SUCCESS) {
        std::cout << "error: could not create rendering pipeline" << std::endl;
        vkDestroyPipelineLayout(g_vkbDevice.device, g_pipelineLayout, nullptr);
    }

    /* it is save to destroy the shader modules after pipeline has been created */
    vkDestroyShaderModule (g_vkbDevice.device, m_shaderModule, nullptr);
   
    
    std::cout << "Render pipeline: " << g_pipeline << std::endl;
}

void VulkanRenderer2D::CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
{
    // std::cout << "Creating vertex buffer..." << std::endl;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferLength;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (vmaCreateBuffer(g_allocator, &bufferInfo, &vmaAllocInfo, &g_vertexBuffer,  &g_vertexBufferAllocation, nullptr) != VK_SUCCESS) {
        std::cout << "error: could not allocate vertex buffer via VMA" << std::endl;
        return;
    }

    void* data;
    vmaMapMemory(g_allocator, g_vertexBufferAllocation, &data);
    std::memcpy(data, bufferData, bufferLength);
    vmaUnmapMemory(g_allocator, g_vertexBufferAllocation);

    g_triangleCount = bufferLength / 3;

    // std::cout << "Vertex buffer: " << m_vertexBuffer << std::endl;
}

void VulkanRenderer2D::CreateIndexBuffer(const std::vector<uint16_t> &bufferData)
{
    // std::cout << "Creating index buffer..." << std::endl;

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

uint32_t VulkanRenderer2D::GetOffset(const uint32_t& uniformIndex, const uint32_t& sizeOfUniform)
{
    // if (uniformIndex == 0)
    //     return 0;

    // // Get device limits
    // wgpu::SupportedLimits deviceSupportedLimits;
    // WebGPU::GetDevice().getLimits(&deviceSupportedLimits);
    // wgpu::Limits deviceLimits = deviceSupportedLimits.limits;
    
    // /** Round 'value' up to the next multiplier of 'step' */
    // auto ceilToNextMultiple = [](uint32_t value, uint32_t step) -> uint32_t
    // {
    //     uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    //     return step * divide_and_ceil;
    // };

    // // Create uniform buffer
    // // Subtility
    // assert(sizeOfUniform > 0);
    // uint32_t uniformStride = ceilToNextMultiple(
    //     (uint32_t)sizeOfUniform,
    //     (uint32_t)deviceLimits.minUniformBufferOffsetAlignment
    // );

    // return uniformStride * uniformIndex;

    return 0;
}

void VulkanRenderer2D::CreateUniformBuffer(size_t bufferLength, uint32_t sizeOfUniform)
{
    // assert(sizeOfUniform > 0);
    // // Create uniform buffer
    // // The buffer will only contain 1 float with the value of uTime
    // wgpu::BufferDescriptor bufferDesc;
    // const size_t maxUniformIndex = bufferLength - 1;
    // bufferDesc.size = sizeOfUniform + GetOffset(maxUniformIndex, sizeOfUniform);
    // // Make sure to flag the buffer as BufferUsage::Uniform
    // bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    // bufferDesc.mappedAtCreation = false;
    // m_uniformBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);
    // m_sizeOfUniform = sizeOfUniform;
}

void VulkanRenderer2D::SetUniformData(const void* bufferData, uint32_t uniformIndex)
{
    // if (m_uniformBuffer)
    // {
    //     // WebGPU::GetQueue().writeBuffer(m_uniformBuffer, offsetof(MyUniforms, time), &bufferData.time, sizeof(MyUniforms::time));
    //     // WebGPU::GetQueue().writeBuffer(m_uniformBuffer, offsetof(MyUniforms, color), &bufferData.color, sizeof(MyUniforms::color));

    //     auto offset = GetOffset(uniformIndex, m_sizeOfUniform);
    //     assert(m_sizeOfUniform > 0);
    //     WebGPU::GetQueue().writeBuffer(m_uniformBuffer, offset, bufferData, m_sizeOfUniform);
    // }
    // else
    // {
    //     assert(false);
    // }
}

void VulkanRenderer2D::SimpleRender()
{
    /* the rendering itself happens here */
    vkCmdBindPipeline(g_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

    /* required for dynamic viewport */
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(g_vkbSwapChain.extent.width);
    viewport.height = static_cast<float>(g_vkbSwapChain.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(g_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = g_vkbSwapChain.extent;
    vkCmdSetScissor(g_commandBuffer, 0, 1, &scissor);

    /* the triangle drawing itself */
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(g_commandBuffer, 0, 1, &g_vertexBuffer, &offset);
    //vkCmdBindDescriptorSets(g_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipelineLayout, 0, 1, &mRenderData.rdDescriptorSet, 0, nullptr);

    vkCmdDraw(g_commandBuffer, g_triangleCount * 3, 1, 0, 0);
}

void VulkanRenderer2D::Render()
{
    // // Set vertex buffer while encoding the render pass
    // m_renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBufferSize);

    // // Set binding group
    // if (m_bindGroup)
    //     m_renderPass.setBindGroup(0, m_bindGroup, 0, nullptr);

    // if (m_indexCount > 0)
    // {
    //     m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexCount * sizeof(uint16_t));
    //     m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
    // }
    // else
    //     m_renderPass.draw(m_vertexCount, 1, 0, 0);
}

void VulkanRenderer2D::RenderIndexed(uint32_t uniformIndex, uint32_t dynamicOffsetCount)
{
    // // Set vertex buffer while encoding the render pass
    // m_renderPass.setVertexBuffer(0, m_vertexBuffer, 0, m_vertexBufferSize);
    // // Set index buffer while encoding the render pass
    // m_renderPass.setIndexBuffer(m_indexBuffer, wgpu::IndexFormat::Uint16, 0, m_indexCount * sizeof(uint16_t));

    // // Set binding group
    // uint32_t dynamicOffset = 0;
    // if (uniformIndex > 0)
    //     dynamicOffset = uniformIndex * GetOffset(1, m_sizeOfUniform);
    
    // m_renderPass.setBindGroup(0, m_bindGroup, dynamicOffsetCount, &dynamicOffset);
    // m_renderPass.drawIndexed(m_indexCount, 1, 0, 0, 0);
}

ImTextureID VulkanRenderer2D::GetDescriptorSet()
{
    return ImTextureID{};
}

void VulkanRenderer2D::BeginRenderPass()
{
    if (vkWaitForFences(g_vkbDevice.device, 1, &g_renderFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        std::cout << "error: waiting for fence failed" << std::endl;
        return;
    }

    if (vkResetFences(g_vkbDevice.device, 1, &g_renderFence) != VK_SUCCESS) {
        std::cout << "error: fence reset failed" << std::endl;
        return;
    }

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(g_vkbDevice.device, g_vkbSwapChain.swapchain, UINT64_MAX, g_presentSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        //return recreateSwapchain();
        std::cout << "need to recreate swapchain" << std::endl;
    } else {
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            std::cout << "error: failed to acquire swapchain image. Error is " << result << std::endl;
        }
    }

    if (vkResetCommandBuffer(g_commandBuffer, 0) != VK_SUCCESS) {
        std::cout << "error: failed to reset command buffer" << std::endl;
        return;
    }

    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if(vkBeginCommandBuffer(g_commandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
        std::cout << "error: failed to begin command buffer" << std::endl;
        return;
    }

    ////

    VkClearValue colorClearValue;
    colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { colorClearValue, depthValue };

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = g_renderpass;

    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = g_vkbSwapChain.extent;
    rpInfo.framebuffer = g_framebuffers[imageIndex];

    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(g_commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer2D::EndRenderPass()
{
	
}

bool VulkanRenderer2D::CreateSwapChain()
{
    vkb::SwapchainBuilder swapChainBuild{g_vkbDevice};

    /* VK_PRESENT_MODE_FIFO_KHR enables vsync */
    auto swapChainBuildRet = swapChainBuild.set_old_swapchain(g_vkbSwapChain).set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).build();
    if (!swapChainBuildRet)
    {
        return false;
    }

    vkb::destroy_swapchain(g_vkbSwapChain);
    g_vkbSwapChain = swapChainBuildRet.value();

    return true;
}

void VulkanRenderer2D::SubmitCommandBuffer()
{

}

} // namespace GraphicsAPI