#include "VulkanRenderer2D.h"
#include "vkb/VkBootstrap.h"
#include <iostream>

#include <Walnut/GraphicsAPI/VulkanGraphics.h>

namespace GraphicsAPI
{

uint32_t g_acquiredFrameIndex = 0;
vkb::Instance g_vkbInstance;
VkSurfaceKHR g_surface = VK_NULL_HANDLE;
vkb::PhysicalDevice g_vkbPhysDevice;
vkb::Device g_vkbDevice;
vkb::Swapchain g_vkbSwapChain;

VkCommandPool g_commandPool = VK_NULL_HANDLE;
VkCommandBuffer g_commandBuffer = VK_NULL_HANDLE;

VkSemaphore g_presentSemaphore = VK_NULL_HANDLE;
VkSemaphore g_renderSemaphore = VK_NULL_HANDLE;
VkFence g_renderFence = VK_NULL_HANDLE;

constexpr VkFormat g_rdDepthFormat = VK_FORMAT_D32_SFLOAT;
VkRenderPass g_renderpass = VK_NULL_HANDLE;

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
    return true;
}

bool getQueue()
{
    return true;
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

    if (!getQueue()) {
        return false;
    }

    if (!CreateSwapChain()) {
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

void VulkanRenderer2D::CreateShaders(const char* shaderSource)
{
//     std::cout << "Creating shader module..." << std::endl;

//     wgpu::ShaderModuleDescriptor shaderDesc;
// #ifdef WEBGPU_BACKEND_WGPU
//     shaderDesc.hintCount = 0;
//     shaderDesc.hints = nullptr;
// #endif

//     wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
//     // Set the chained struct's header
//     shaderCodeDesc.chain.next = nullptr;
//     shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
//     shaderCodeDesc.code = shaderSource;
//     // Connect the chain
//     shaderDesc.nextInChain = &shaderCodeDesc.chain;
//     m_shaderModule = WebGPU::GetDevice().createShaderModule(shaderDesc);

//     std::cout << "Shader module: " << m_shaderModule << std::endl;
}

void VulkanRenderer2D::CreateStandaloneShader(const char *shaderSource, uint32_t vertexShaderCallCount)
{
    // CreateShaders(shaderSource);
    // m_vertexCount = vertexShaderCallCount;
}

void VulkanRenderer2D::CreatePipeline()
{
    // std::cout << "Creating render pipeline..." << std::endl;

    // wgpu::RenderPipelineDescriptor pipelineDesc;

    // // Vertex fetch
    // if (m_vertexBufferSize > 0)
    // {
    //     pipelineDesc.vertex.bufferCount = 1;
    //     pipelineDesc.vertex.buffers = &m_vertexBufferLayout;
    // }
    // else
    // {
    //     pipelineDesc.vertex.bufferCount = 0;
    //     pipelineDesc.vertex.buffers = nullptr;
    // }
    // // Vertex shader
    // pipelineDesc.vertex.module = m_shaderModule;
	// pipelineDesc.vertex.entryPoint = "vs_main";
    // pipelineDesc.vertex.constantCount = 0;
	// pipelineDesc.vertex.constants = nullptr;

    // // Primitive assembly and rasterization
	// // Each sequence of 3 vertices is considered as a triangle
	// pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
	// // We'll see later how to specify the order in which vertices should be
	// // connected. When not specified, vertices are considered sequentially.
	// pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
	// // The face orientation is defined by assuming that when looking
	// // from the front of the face, its corner vertices are enumerated
	// // in the counter-clockwise (CCW) order.
	// pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;
	// // But the face orientation does not matter much because we do not
	// // cull (i.e. "hide") the faces pointing away from us (which is often
	// // used for optimization).
	// pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    // // Fragment shader
	// wgpu::FragmentState fragmentState;
	// pipelineDesc.fragment = &fragmentState;
	// fragmentState.module = m_shaderModule;
	// fragmentState.entryPoint = "fs_main";
	// fragmentState.constantCount = 0;
	// fragmentState.constants = nullptr;

    // // Configure blend state
	// wgpu::BlendState blendState;
	// // Usual alpha blending for the color:
	// blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
	// blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
	// blendState.color.operation = wgpu::BlendOperation::Add;
	// // We leave the target alpha untouched:
	// blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
	// blendState.alpha.dstFactor = wgpu::BlendFactor::One;
	// blendState.alpha.operation = wgpu::BlendOperation::Add;

    // wgpu::ColorTargetState colorTarget;
	// colorTarget.format = WebGPU::GetSwapChainFormat();
	// colorTarget.blend = &blendState;
	// colorTarget.writeMask = wgpu::ColorWriteMask::All; // We could write to only some of the color channels.

	// // We have only one target because our render pass has only one output color
	// // attachment.
	// fragmentState.targetCount = 1;
	// fragmentState.targets = &colorTarget;
	
	// // Depth and stencil tests are not used here
	// pipelineDesc.depthStencil = nullptr;

    // // Multi-sampling
	// // Samples per pixel
	// pipelineDesc.multisample.count = 1;
	// // Default value for the mask, meaning "all bits on"
	// pipelineDesc.multisample.mask = ~0u;
	// // Default value as well (irrelevant for count = 1 anyways)
	// pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// // Pipeline layout
    // if (m_pipelineLayout)
	//     pipelineDesc.layout = m_pipelineLayout;
    // else
    //     pipelineDesc.layout = nullptr;

    // m_pipeline = WebGPU::GetDevice().createRenderPipeline(pipelineDesc);
    // std::cout << "Render pipeline: " << m_pipeline << std::endl;
}

void VulkanRenderer2D::CreateVertexBuffer(const void* bufferData, uint32_t bufferLength, RenderSys::VertexBufferLayout bufferLayout)
{
    // std::cout << "Creating vertex buffer..." << std::endl;
    // m_vertexCount = bufferLength / bufferLayout.arrayStride;
    // m_vertexBufferSize = bufferLength;
    // m_vertexBufferLayout = bufferLayout;
    // wgpu::BufferDescriptor bufferDesc;
    // bufferDesc.size = m_vertexBufferSize;
    // bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
    // bufferDesc.mappedAtCreation = false;
    // bufferDesc.label = "Vertex Buffer";
    // m_vertexBuffer = WebGPU::GetDevice().createBuffer(bufferDesc);

    // // Upload vertex data to the buffer
    // WebGPU::GetQueue().writeBuffer(m_vertexBuffer, 0, bufferData, bufferDesc.size);
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

void VulkanRenderer2D::SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    // Create a bind group layout
	// wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
	// bindGroupLayoutDesc.entryCount = 1;
	// bindGroupLayoutDesc.entries = &bindGroupLayoutEntry;
	// m_bindGroupLayout = WebGPU::GetDevice().createBindGroupLayout(bindGroupLayoutDesc);
}

void VulkanRenderer2D::CreateBindGroup()
{
    // if (m_bindGroupLayout)
    // {
    //     // Create the pipeline layout
    //     wgpu::PipelineLayoutDescriptor pipelineLayoutDesc;
    //     pipelineLayoutDesc.bindGroupLayoutCount = 1;
    //     pipelineLayoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&m_bindGroupLayout;
    //     m_pipelineLayout = WebGPU::GetDevice().createPipelineLayout(pipelineLayoutDesc);



    //     // Create a binding
    //     wgpu::BindGroupEntry binding;
    //     // The index of the binding (the entries in bindGroupDesc can be in any order)
    //     binding.binding = 0;
    //     // The buffer it is actually bound to
    //     binding.buffer = m_uniformBuffer;
    //     // We can specify an offset within the buffer, so that a single buffer can hold
    //     // multiple uniform blocks.
    //     binding.offset = 0;
    //     // And we specify again the size of the buffer.
    //     assert(m_sizeOfUniform > 0);
    //     binding.size = m_sizeOfUniform;


    //     // A bind group contains one or multiple bindings
    //     wgpu::BindGroupDescriptor bindGroupDesc;
    //     bindGroupDesc.layout = m_bindGroupLayout;
    //     // There must be as many bindings as declared in the layout!
    //     bindGroupDesc.entryCount = 1; // TODO: Nisal - use bindGroupLayoutDesc.entryCount
    //     bindGroupDesc.entries = &binding;
    //     m_bindGroup = WebGPU::GetDevice().createBindGroup(bindGroupDesc);
    // }
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
    // vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdPipeline);

    // /* required for dynamic viewport */
    // vkCmdSetViewport(mRenderData.rdCommandBuffer, 0, 1, &viewport);
    // vkCmdSetScissor(mRenderData.rdCommandBuffer, 0, 1, &scissor);

    // /* the triangle drawing itself */
    // VkDeviceSize offset = 0;
    // vkCmdBindVertexBuffers(mRenderData.rdCommandBuffer, 0, 1, &mVertexBuffer, &offset);
    // vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdPipelineLayout, 0, 1, &mRenderData.rdDescriptorSet, 0, nullptr);

    // vkCmdDraw(mRenderData.rdCommandBuffer, mTriangleCount * 3, 1, 0, 0);
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
    rpInfo.framebuffer = mRenderData.rdFramebuffers[imageIndex];

    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

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