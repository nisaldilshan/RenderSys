#include "VulkanRenderer2D.h"

#include <iostream>
#include <fstream>
#include "vkb/VkBootstrap.h"
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

namespace GraphicsAPI
{

uint32_t g_acquiredFrameIndex = 0;
//vkb::Swapchain g_vkbSwapChain;

VmaAllocator g_allocator;

VkCommandPool g_commandPool = VK_NULL_HANDLE;
//VkCommandBuffer g_commandBuffer = VK_NULL_HANDLE;

VkSemaphore g_presentSemaphore = VK_NULL_HANDLE;
VkSemaphore g_renderSemaphore = VK_NULL_HANDLE;
VkFence g_renderFence = VK_NULL_HANDLE;

std::vector<VkImageView> g_rdSwapchainImageViews;
std::vector<VkFramebuffer> g_framebuffers;

//VkQueue g_graphicsQueue = VK_NULL_HANDLE;
//VkQueue g_presentQueue = VK_NULL_HANDLE;

VkDeviceMemory m_Memory;
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

bool m_inited = false;

// bool createCommandBuffer()
// {
//     VkCommandPoolCreateInfo poolCreateInfo{};
//     poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//     poolCreateInfo.queueFamilyIndex = Vulkan::GetQueueFamilyIndex();
//     poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

//     if (vkCreateCommandPool(Vulkan::GetDevice(), &poolCreateInfo, nullptr, &g_commandPool) != VK_SUCCESS) {
//         std::cout << "error: could not create command pool" << std::endl;
//         return false;
//     }

//     VkCommandBufferAllocateInfo bufferAllocInfo{};
//     bufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//     bufferAllocInfo.commandPool = g_commandPool;
//     bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//     bufferAllocInfo.commandBufferCount = 1;

//     if (vkAllocateCommandBuffers(Vulkan::GetDevice(), &bufferAllocInfo, &g_commandBuffer) != VK_SUCCESS) {
//         std::cout << "error: could not allocate command buffers" << std::endl;
//         return false;
//     }

//     return true;

//     // call vkDestroyCommandPool(renderData.rdVkbDevice.device, renderData.rdCommandPool, nullptr); to destroy commandpool
//     // call vkFreeCommandBuffers(renderData.rdVkbDevice.device, renderData.rdCommandPool, 1, &commandBuffer); to destroy command buffer
// }

bool createRenderPass() 
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
  //subpassDesc.pDepthStencilAttachment = &depthAttRef;

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
  VkAttachmentDescription attachments[] = { colorAtt };

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = attachments;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
//   renderPassInfo.dependencyCount = 2;
//   renderPassInfo.pDependencies = dependencies;

  if (vkCreateRenderPass(Vulkan::GetDevice(), &renderPassInfo, nullptr, &g_renderpass) != VK_SUCCESS) {
    std::cout<< "error; could not create renderpass" << std::endl;
    return false;
  }

  return true;
}


bool createDepthBuffer() 
{
    // VkImageCreateInfo depthImageInfo{};
    // depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    // depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    // depthImageInfo.format = g_rdDepthFormat;
    // //depthImageInfo.extent = depthImageExtent;
    // depthImageInfo.mipLevels = 1;
    // depthImageInfo.arrayLayers = 1;
    // depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    // depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = VK_FORMAT_R8G8B8A8_UNORM;
    info.extent.width = 10;//m_width;
    info.extent.height = 10;//m_height;
    info.extent.depth = 1;
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult err = vkCreateImage(Vulkan::GetDevice(), &info, nullptr, &g_depthImage);
    Vulkan::check_vk_result(err);
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(Vulkan::GetDevice(), g_depthImage, &req);
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(Vulkan::GetPhysicalDevice(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
    err = vkAllocateMemory(Vulkan::GetDevice(), &alloc_info, nullptr, &m_Memory);
    Vulkan::check_vk_result(err);
    err = vkBindImageMemory(Vulkan::GetDevice(), g_depthImage, m_Memory, 0);
    Vulkan::check_vk_result(err);

    VkImageViewCreateInfo depthImageViewinfo{};
    depthImageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewinfo.image = g_depthImage;
    depthImageViewinfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    depthImageViewinfo.subresourceRange.baseMipLevel = 0;
    depthImageViewinfo.subresourceRange.levelCount = 1;
    depthImageViewinfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewinfo.subresourceRange.layerCount = 1;
    depthImageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (vkCreateImageView(Vulkan::GetDevice(), &depthImageViewinfo, nullptr, &g_depthImageView) != VK_SUCCESS) {
        std::cout << "error: could not create depth buffer image view" << std::endl;
        return false;
    }
    return true;
}

bool createSyncObjects()
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(Vulkan::GetDevice(), &semaphoreInfo, nullptr, &g_presentSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(Vulkan::GetDevice(), &semaphoreInfo, nullptr, &g_renderSemaphore) != VK_SUCCESS ||
        vkCreateFence(Vulkan::GetDevice(), &fenceInfo, nullptr, &g_renderFence) != VK_SUCCESS) {
        std::cout << "error: failed to init sync objects" << std::endl;
        return false;
    }
    return true;
}


bool getQueue() {
//   auto graphQueueRet = g_vkbDevice.get_queue(vkb::QueueType::graphics);
//   if (!graphQueueRet.has_value()) {
//     std::cout << "error: could not get graphics queue" << std::endl;
//     return false;
//   }
//   g_graphicsQueue = graphQueueRet.value();

//   auto presentQueueRet = g_vkbDevice.get_queue(vkb::QueueType::present);
//   if (!presentQueueRet.has_value()) {
//     std::cout << "error: could not get present queue" << std::endl;
//     return false;
//   }
//   g_presentQueue = presentQueueRet.value();

  return true;
}

bool VulkanRenderer2D::Init()
{
    if (!createDepthBuffer()) {
        return false;
    }

    // if (!createCommandBuffer())
    // {
    //     return false;
    // }

    if (!createRenderPass())
    {
        return false;
    }

    if (!createSyncObjects()) {
        return false;
    }

    CreateBindGroup();

    m_inited = true;
    return true;
}

bool VulkanRenderer2D::CreateSwapChain()
{
    // vkb::SwapchainBuilder swapChainBuild{g_vkbDevice};

    // /* VK_PRESENT_MODE_FIFO_KHR enables vsync */
    // auto swapChainBuildRet = swapChainBuild.set_old_swapchain(g_vkbSwapChain).set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR).build();
    // if (!swapChainBuildRet)
    // {
    //     return false;
    // }

    // vkb::destroy_swapchain(g_vkbSwapChain);
    // g_vkbSwapChain = swapChainBuildRet.value();

    return true;
}
void VulkanRenderer2D::CreateTextureToRenderInto(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
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


std::string loadFileToString(std::string fileName) {
  std::ifstream inFile(fileName, std::ios::binary);
  std::string str;

  if (inFile.is_open()) {
    str.clear();
    // allocate string data (no slower realloc)
    inFile.seekg(0, std::ios::end);
    str.reserve(inFile.tellg());
    inFile.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(inFile)),
                std::istreambuf_iterator<char>());
    inFile.close();
  } else {
    //Logger::log(1, "%s error: could not open file %s\n", __FUNCTION__, fileName.c_str());
    //Logger::log(1, "%s error: system says '%s'\n", __FUNCTION__, strerror(errno));
    return std::string();
  }

  if (inFile.bad() || inFile.fail()) {
    //Logger::log(1, "%s error: error while reading file %s\n", __FUNCTION__, fileName.c_str());
    inFile.close();
    return std::string();
  }

  inFile.close();
  //Logger::log(1, "%s: file %s successfully read to string\n", __FUNCTION__, fileName.c_str());
  return str;
}

void VulkanRenderer2D::CreateShaders(const char* shaderSource)
{
    std::cout << "Creating shader modules..." << std::endl;

    // auto file = std::string(shaderSource);
    // auto shaderAsBinaryString = loadFileToString(file); 

    {
        // ./glslangValidator -V -x -o shader.vert.u32 shader.vert
        static uint32_t __glsl_shader_vert_spv[] =
        {
            0x07230203,0x00010000,0x0008000b,0x00000021,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000012,0x0000001d,
            0x0000001f,0x00030003,0x00000002,0x000001cc,0x00040005,0x00000004,0x6e69616d,0x00000000,
            0x00060005,0x0000000b,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000000b,
            0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000000b,0x00000001,0x505f6c67,
            0x746e696f,0x657a6953,0x00000000,0x00070006,0x0000000b,0x00000002,0x435f6c67,0x4470696c,
            0x61747369,0x0065636e,0x00070006,0x0000000b,0x00000003,0x435f6c67,0x446c6c75,0x61747369,
            0x0065636e,0x00030005,0x0000000d,0x00000000,0x00040005,0x00000012,0x736f5061,0x00000000,
            0x00050005,0x0000001d,0x43786574,0x64726f6f,0x00000000,0x00050005,0x0000001f,0x78655461,
            0x726f6f43,0x00000064,0x00050048,0x0000000b,0x00000000,0x0000000b,0x00000000,0x00050048,
            0x0000000b,0x00000001,0x0000000b,0x00000001,0x00050048,0x0000000b,0x00000002,0x0000000b,
            0x00000003,0x00050048,0x0000000b,0x00000003,0x0000000b,0x00000004,0x00030047,0x0000000b,
            0x00000002,0x00040047,0x00000012,0x0000001e,0x00000000,0x00040047,0x0000001d,0x0000001e,
            0x00000000,0x00040047,0x0000001f,0x0000001e,0x00000001,0x00020013,0x00000002,0x00030021,
            0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,
            0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,0x00000008,0x00000009,
            0x00000001,0x0004001c,0x0000000a,0x00000006,0x00000009,0x0006001e,0x0000000b,0x00000007,
            0x00000006,0x0000000a,0x0000000a,0x00040020,0x0000000c,0x00000003,0x0000000b,0x0004003b,
            0x0000000c,0x0000000d,0x00000003,0x00040015,0x0000000e,0x00000020,0x00000001,0x0004002b,
            0x0000000e,0x0000000f,0x00000000,0x00040017,0x00000010,0x00000006,0x00000003,0x00040020,
            0x00000011,0x00000001,0x00000010,0x0004003b,0x00000011,0x00000012,0x00000001,0x0004002b,
            0x00000006,0x00000014,0x3f800000,0x00040020,0x00000019,0x00000003,0x00000007,0x00040017,
            0x0000001b,0x00000006,0x00000002,0x00040020,0x0000001c,0x00000003,0x0000001b,0x0004003b,
            0x0000001c,0x0000001d,0x00000003,0x00040020,0x0000001e,0x00000001,0x0000001b,0x0004003b,
            0x0000001e,0x0000001f,0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,
            0x000200f8,0x00000005,0x0004003d,0x00000010,0x00000013,0x00000012,0x00050051,0x00000006,
            0x00000015,0x00000013,0x00000000,0x00050051,0x00000006,0x00000016,0x00000013,0x00000001,
            0x00050051,0x00000006,0x00000017,0x00000013,0x00000002,0x00070050,0x00000007,0x00000018,
            0x00000015,0x00000016,0x00000017,0x00000014,0x00050041,0x00000019,0x0000001a,0x0000000d,
            0x0000000f,0x0003003e,0x0000001a,0x00000018,0x0004003d,0x0000001b,0x00000020,0x0000001f,
            0x0003003e,0x0000001d,0x00000020,0x000100fd,0x00010038
        };

        VkShaderModuleCreateInfo shaderCreateInfoVert{};
        shaderCreateInfoVert.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfoVert.codeSize = sizeof(__glsl_shader_vert_spv);
        shaderCreateInfoVert.pCode = reinterpret_cast<const uint32_t*>(__glsl_shader_vert_spv);

        if (vkCreateShaderModule(Vulkan::GetDevice(), &shaderCreateInfoVert, nullptr, &m_shaderModuleVertex) != VK_SUCCESS) {
            std::cout << "could not load vertex shader" << std::endl;
            return;
        }

        std::cout << "Vertex Shader module: " << m_shaderModuleVertex << std::endl;
    }


    {
        // ./glslangValidator -V -x -o shader.frag.u32 shader.frag
        static uint32_t __glsl_shader_frag_spv[] = 
        {
            0x07230203,0x00010000,0x0008000b,0x00000011,0x00000000,0x00020011,0x00000001,0x0006000b,
            0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
            0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x00000010,0x00030010,
            0x00000004,0x00000007,0x00030003,0x00000002,0x000001cc,0x00040005,0x00000004,0x6e69616d,
            0x00000000,0x00050005,0x00000009,0x67617246,0x6f6c6f43,0x00000072,0x00050005,0x00000010,
            0x43786574,0x64726f6f,0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,
            0x00000010,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
            0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,
            0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x0004002b,
            0x00000006,0x0000000a,0x3f800000,0x0004002b,0x00000006,0x0000000b,0x3ecccccd,0x0004002b,
            0x00000006,0x0000000c,0x00000000,0x0007002c,0x00000007,0x0000000d,0x0000000a,0x0000000b,
            0x0000000c,0x0000000a,0x00040017,0x0000000e,0x00000006,0x00000002,0x00040020,0x0000000f,
            0x00000001,0x0000000e,0x0004003b,0x0000000f,0x00000010,0x00000001,0x00050036,0x00000002,
            0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0003003e,0x00000009,0x0000000d,
            0x000100fd,0x00010038
        };

        VkShaderModuleCreateInfo shaderCreateInfoFrag{};
        shaderCreateInfoFrag.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfoFrag.codeSize = sizeof(__glsl_shader_frag_spv);
        shaderCreateInfoFrag.pCode = reinterpret_cast<const uint32_t*>(__glsl_shader_frag_spv);

        if (vkCreateShaderModule(Vulkan::GetDevice(), &shaderCreateInfoFrag, nullptr, &m_shaderModuleFragment) != VK_SUCCESS) {
            std::cout << "could not load fragment shader" << std::endl;
            return;
        }

        std::cout << "Fragment Shader module: " << m_shaderModuleFragment << std::endl;
    }
}

void VulkanRenderer2D::CreateStandaloneShader(const char *shaderSource, uint32_t vertexShaderCallCount)
{
    CreateShaders(shaderSource);
    m_vertexCount = vertexShaderCallCount;
}

void VulkanRenderer2D::SetBindGroupLayoutEntry(RenderSys::BindGroupLayoutEntry bindGroupLayoutEntry)
{
    
}

void VulkanRenderer2D::CreateBindGroup()
{
    // Create a bind group layout
    m_bindGroupLayout = std::make_unique<VkPipelineLayoutCreateInfo>();
    m_bindGroupLayout->sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    m_bindGroupLayout->setLayoutCount = 0; // was 1
    //m_bindGroupLayout->pSetLayouts = &renderData.rdTextureLayout;
    m_bindGroupLayout->pushConstantRangeCount = 0;

    if (m_bindGroupLayout)
    {
        if (vkCreatePipelineLayout(Vulkan::GetDevice(), m_bindGroupLayout.get(), nullptr, &g_pipelineLayout) != VK_SUCCESS) {
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

    if (m_shaderModuleVertex == VK_NULL_HANDLE || m_shaderModuleFragment == VK_NULL_HANDLE) {
        std::cout << "error: could not load shaders" << std::endl;
        return;
    }

    VkPipelineShaderStageCreateInfo vertexStageInfo{};
    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStageInfo.module = m_shaderModuleVertex;
    vertexStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStageInfo{};
    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = m_shaderModuleFragment;
    fragmentStageInfo.pName = "main";

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

    if (vkCreateGraphicsPipelines(Vulkan::GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &g_pipeline) != VK_SUCCESS) {
        std::cout << "error: could not create rendering pipeline" << std::endl;
        vkDestroyPipelineLayout(Vulkan::GetDevice(), g_pipelineLayout, nullptr);
    }

    /* it is save to destroy the shader modules after pipeline has been created */
    vkDestroyShaderModule (Vulkan::GetDevice(), m_shaderModuleVertex, nullptr);
    vkDestroyShaderModule (Vulkan::GetDevice(), m_shaderModuleFragment, nullptr);
    
    std::cout << "Render pipeline: " << g_pipeline << std::endl;
}

void VulkanRenderer2D::CreateFrameBuffer()
{
    // g_rdSwapchainImageViews = g_vkbSwapChain.get_image_views().value();

    // g_framebuffers.resize(g_rdSwapchainImageViews.size());

    // for (unsigned int i = 0; i < g_rdSwapchainImageViews.size(); ++i) {
    //     VkImageView attachments[] = { g_rdSwapchainImageViews.at(i), g_depthImageView };

    //     VkFramebufferCreateInfo FboInfo{};
    //     FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    //     FboInfo.renderPass = g_renderpass;
    //     FboInfo.attachmentCount = 2;
    //     FboInfo.pAttachments = attachments;
    //     FboInfo.width = m_width;
    //     FboInfo.height = m_height;
    //     FboInfo.layers = 1;

    //     if (vkCreateFramebuffer(Vulkan::GetDevice(), &FboInfo, nullptr, &g_framebuffers[i]) != VK_SUCCESS) {
    //         std::cout << "error: failed to create framebuffer" << std::endl;
    //         return ;
    //     }
    // }

    // do destroy
    // for (auto &fb : renderData.rdFramebuffers) {
    //     vkDestroyFramebuffer(renderData.rdVkbDevice.device, fb, nullptr);
    // }
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
    std::cout << "Simple render" << std::endl;
}

void VulkanRenderer2D::Render()
{
    // /* the rendering itself happens here */
    // vkCmdBindPipeline(g_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

    // /* required for dynamic viewport */
    // VkViewport viewport{};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = static_cast<float>(m_width);
    // viewport.height = static_cast<float>(m_height);
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;
    // vkCmdSetViewport(g_commandBuffer, 0, 1, &viewport);

    // VkRect2D scissor{};
    // scissor.offset = { 0, 0 };
    // scissor.extent = { m_width, m_height };
    // vkCmdSetScissor(g_commandBuffer, 0, 1, &scissor);

    // /* the triangle drawing itself */
    // VkDeviceSize offset = 0;
    // vkCmdBindVertexBuffers(g_commandBuffer, 0, 1, &g_vertexBuffer, &offset);
    // //vkCmdBindDescriptorSets(g_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipelineLayout, 0, 1, &mRenderData.rdDescriptorSet, 0, nullptr);

    // vkCmdDraw(g_commandBuffer, g_triangleCount * 3, 1, 0, 0);
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

VkSampler g_TextureSampler = VK_NULL_HANDLE;
void CreateSampler()
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

    if (vkCreateSampler(Vulkan::GetDevice(), &texSamplerInfo, nullptr, &g_TextureSampler) != VK_SUCCESS) {
        std::cout << "error: could not create sampler for texture" << std::endl;
    }
}

VkImage m_Image1;
VkImageView attachment1;
ImTextureID VulkanRenderer2D::GetDescriptorSet()
{
    if (!m_inited)
        return ImTextureID{};
    static bool samplerCreated = false;
    if (!samplerCreated && Vulkan::GetDevice() != VK_NULL_HANDLE) 
    {
        CreateSampler();
        samplerCreated = true;
    }
    static auto finalDescriptorSet = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(g_TextureSampler, attachment1, VK_IMAGE_LAYOUT_GENERAL);
    if (samplerCreated)
        return finalDescriptorSet;
    else
        return ImTextureID{};
}


//uint32_t g_acquiredimageIndex = 0;
VkCommandBuffer commandBufferForReal;
void VulkanRenderer2D::BeginRenderPass()
{
    // if (vkWaitForFences(Vulkan::GetDevice(), 1, &g_renderFence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
    //     std::cout << "error: waiting for fence failed" << std::endl;
    //     return;
    // }

    // if (vkResetFences(Vulkan::GetDevice(), 1, &g_renderFence) != VK_SUCCESS) {
    //     std::cout << "error: fence reset failed" << std::endl;
    //     return;
    // }

    // VkResult result = vkAcquireNextImageKHR(Vulkan::GetDevice(), g_vkbSwapChain.swapchain, UINT64_MAX, g_presentSemaphore, VK_NULL_HANDLE, &g_acquiredimageIndex);
    // if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //     //return recreateSwapchain();
    //     std::cout << "need to recreate swapchain" << std::endl;
    // } else {
    //     if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    //         std::cout << "error: failed to acquire swapchain image. Error is " << result << std::endl;
    //     }
    // }

    // if (vkResetCommandBuffer(g_commandBuffer, 0) != VK_SUCCESS) {
    //     std::cout << "error: failed to reset command buffer" << std::endl;
    //     return;
    // }

    // VkCommandBufferBeginInfo cmdBeginInfo{};
    // cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    // if(vkBeginCommandBuffer(g_commandBuffer, &cmdBeginInfo) != VK_SUCCESS) {
    //     std::cout << "error: failed to begin command buffer" << std::endl;
    //     return;
    // }

    ////

    VkClearValue colorClearValue;
    colorClearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

    VkClearValue depthValue;
    depthValue.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { colorClearValue, depthValue };

    static bool frameBufferAttachmentsCreated = false;
    static VkFramebuffer frameBufferAttachment;
    
    //static VkImage m_Image2;
    if (!frameBufferAttachmentsCreated) 
    {
        
        {
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
            VkResult err = vkCreateImage(Vulkan::GetDevice(), &info, nullptr, &m_Image1);
            Vulkan::check_vk_result(err);
            VkMemoryRequirements req;
            vkGetImageMemoryRequirements(Vulkan::GetDevice(), m_Image1, &req);
            VkMemoryAllocateInfo alloc_info = {};
            alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize = req.size;
            alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(Vulkan::GetPhysicalDevice(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
            err = vkAllocateMemory(Vulkan::GetDevice(), &alloc_info, nullptr, &m_Memory);
            Vulkan::check_vk_result(err);
            err = vkBindImageMemory(Vulkan::GetDevice(), m_Image1, m_Memory, 0);
            Vulkan::check_vk_result(err);

            VkImageViewCreateInfo viewinfo = {};
            viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewinfo.image = m_Image1;
            viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewinfo.format = VK_FORMAT_R8G8B8A8_UNORM;
            viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewinfo.subresourceRange.levelCount = 1;
            viewinfo.subresourceRange.layerCount = 1;
            err = vkCreateImageView(Vulkan::GetDevice(), &viewinfo, nullptr, &attachment1);
            Vulkan::check_vk_result(err);
        }

        // VkImageView attachment2;
        // {
        //     static VkDeviceMemory m_Memory;
        //     VkImageCreateInfo info = {};
        //     info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        //     info.imageType = VK_IMAGE_TYPE_2D;
        //     info.format = g_rdDepthFormat;
        //     info.extent.width = m_width;
        //     info.extent.height = m_height;
        //     info.extent.depth = 1;
        //     info.mipLevels = 1;
        //     info.arrayLayers = 1;
        //     info.samples = VK_SAMPLE_COUNT_1_BIT;
        //     info.tiling = VK_IMAGE_TILING_OPTIMAL;
        //     info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;
        //     info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        //     info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //     VkResult err = vkCreateImage(Vulkan::GetDevice(), &info, nullptr, &m_Image2);
        //     Vulkan::check_vk_result(err);
        //     VkMemoryRequirements req;
        //     vkGetImageMemoryRequirements(Vulkan::GetDevice(), m_Image2, &req);
        //     VkMemoryAllocateInfo alloc_info = {};
        //     alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        //     alloc_info.allocationSize = req.size;
        //     alloc_info.memoryTypeIndex = Utils::GetVulkanMemoryType(Vulkan::GetPhysicalDevice(), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, req.memoryTypeBits);
        //     err = vkAllocateMemory(Vulkan::GetDevice(), &alloc_info, nullptr, &m_Memory);
        //     Vulkan::check_vk_result(err);
        //     err = vkBindImageMemory(Vulkan::GetDevice(), m_Image2, m_Memory, 0);
        //     Vulkan::check_vk_result(err);

        //     VkImageViewCreateInfo viewinfo = {};
        //     viewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        //     viewinfo.image = m_Image2;
        //     viewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        //     viewinfo.format = g_rdDepthFormat;
        //     viewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        //     viewinfo.subresourceRange.levelCount = 1;
        //     viewinfo.subresourceRange.layerCount = 1;
        //     err = vkCreateImageView(Vulkan::GetDevice(), &viewinfo, nullptr, &attachment2);
        //     Vulkan::check_vk_result(err);
        // }

        VkImageView attachments1[] = { attachment1 };
        
        VkFramebufferCreateInfo FboInfo{};
        FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FboInfo.renderPass = g_renderpass;
        FboInfo.attachmentCount = 1;
        FboInfo.pAttachments = attachments1;
        FboInfo.width = m_width;
        FboInfo.height = m_height;
        FboInfo.layers = 1;

        if (vkCreateFramebuffer(Vulkan::GetDevice(), &FboInfo, nullptr, &frameBufferAttachment) != VK_SUCCESS) {
            std::cout << "error: failed to create framebuffer" << std::endl;
            return ;
        }
        frameBufferAttachmentsCreated = true;
    }

    commandBufferForReal = GraphicsAPI::Vulkan::GetCommandBuffer(true);

    VkImageMemoryBarrier copy_barrier = {};
    copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    copy_barrier.image = m_Image1;
    copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_barrier.subresourceRange.levelCount = 1;
    copy_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBufferForReal, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copy_barrier);

    VkImageMemoryBarrier use_barrier = {};
    use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    use_barrier.image = m_Image1;
    use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    use_barrier.subresourceRange.levelCount = 1;
    use_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBufferForReal, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);
    // {

    //     VkImageMemoryBarrier copy_barrier = {};
    //     copy_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //     copy_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //     copy_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //     copy_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //     copy_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     copy_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     copy_barrier.image = m_Image2;
    //     copy_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //     copy_barrier.subresourceRange.levelCount = 1;
    //     copy_barrier.subresourceRange.layerCount = 1;
    //     vkCmdPipelineBarrier(commandBufferForReal, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copy_barrier);

    //     VkImageMemoryBarrier use_barrier = {};
    //     use_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //     use_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //     use_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //     use_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    //     use_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //     use_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     use_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //     use_barrier.image = m_Image2;
    //     use_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    //     use_barrier.subresourceRange.levelCount = 1;
    //     use_barrier.subresourceRange.layerCount = 1;
    //     vkCmdPipelineBarrier(commandBufferForReal, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &use_barrier);
    // }

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = g_renderpass;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent = { m_width, m_height };
    rpInfo.framebuffer = frameBufferAttachment;
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

    
    vkCmdBeginRenderPass(commandBufferForReal, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer2D::EndRenderPass()
{
    

	vkCmdEndRenderPass(commandBufferForReal);

    // if (vkEndCommandBuffer(g_commandBuffer) != VK_SUCCESS) {
    //     std::cout << "error: failed to end command buffer" << std::endl;
    // }

    //SubmitCommandBuffer();

    GraphicsAPI::Vulkan::FlushCommandBuffer(commandBufferForReal);
}

void VulkanRenderer2D::SubmitCommandBuffer()
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;
    // submitInfo.waitSemaphoreCount = 1;
    // submitInfo.pWaitSemaphores = &g_presentSemaphore;
    // submitInfo.signalSemaphoreCount = 1;
    // submitInfo.pSignalSemaphores = &g_renderSemaphore;
    submitInfo.commandBufferCount = 1;
    //submitInfo.pCommandBuffers = &g_commandBuffer;

    if (vkQueueSubmit(Vulkan::GetDeviceQueue(), 1, &submitInfo, g_renderFence) != VK_SUCCESS) {
        std::cout << "error: failed to submit draw command buffer" << std::endl;
        return;
    }

    // VkPresentInfoKHR presentInfo{};
    // presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // presentInfo.waitSemaphoreCount = 1;
    // presentInfo.pWaitSemaphores = &g_renderSemaphore;
    // presentInfo.swapchainCount = 1;
    // presentInfo.pSwapchains = &g_vkbSwapChain.swapchain;
    // presentInfo.pImageIndices = &g_acquiredimageIndex;

    // VkResult result = vkQueuePresentKHR(g_presentQueue, &presentInfo);
    // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    //     std::cout << "need to recreate swapchain" << std::endl;
    //     //return recreateSwapchain();
    // } else {
    //     if (result != VK_SUCCESS) {
    //         std::cout << "error: failed to present swapchain image" << std::endl;
    //     }
    // }
}

} // namespace GraphicsAPI