#include "VulkanRendererUtils.h"
#include <stdexcept>

namespace RenderSys
{
namespace Vulkan 
{

VkCommandPool g_commandPool = VK_NULL_HANDLE;
void CreateCommandPool()
{
    auto queueFamilyIndices = GraphicsAPI::Vulkan::FindQueueFamilies();
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    auto err = vkCreateCommandPool(GraphicsAPI::Vulkan::GetDevice(), &poolInfo, nullptr, &g_commandPool);
    GraphicsAPI::Vulkan::check_vk_result(err);
}

VkCommandPool GetCommandPool()
{
    return g_commandPool;
}

void DestroyCommandPool()
{
    vkDeviceWaitIdle(GraphicsAPI::Vulkan::GetDevice());
    // when you destroy a command pool, all command buffers allocated from that pool are automatically destroyed
    vkDestroyCommandPool(GraphicsAPI::Vulkan::GetDevice(), g_commandPool, nullptr);
    g_commandPool = VK_NULL_HANDLE;
}


VkFormat RenderSysFormatToVulkanFormat(RenderSys::VertexFormat format)
{
    switch (format)
    {
    case RenderSys::VertexFormat::Float32x2: return VK_FORMAT_R32G32_SFLOAT;
    case RenderSys::VertexFormat::Float32x3: return VK_FORMAT_R32G32B32_SFLOAT;
    default: assert(false);
    }
    return (VkFormat)0;
}

std::pair<int, VkDeviceSize> FindAppropriateMemoryType(const VkBuffer& buffer, unsigned int flags)
{
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(GraphicsAPI::Vulkan::GetDevice(), buffer, &mem_reqs);

    VkPhysicalDeviceMemoryProperties gpu_mem;
    vkGetPhysicalDeviceMemoryProperties(GraphicsAPI::Vulkan::GetPhysicalDevice(), &gpu_mem);

    int mem_type_idx = -1;
    for (int j = 0; j < gpu_mem.memoryTypeCount; j++) {
        if ((mem_reqs.memoryTypeBits & (1 << j)) &&
            (gpu_mem.memoryTypes[j].propertyFlags & flags) == flags)
        {
            mem_type_idx = j;
            break;
        }
    }

    return std::make_pair(mem_type_idx, mem_reqs.size);
}

VkShaderStageFlags GetVulkanShaderStageVisibility(RenderSys::ShaderStage shaderStage)
{
    VkShaderStageFlags result;
    if (static_cast<uint32_t>(shaderStage) == 1) // RenderSys::ShaderStage::Vertex
    {
        result = VK_SHADER_STAGE_VERTEX_BIT;
    }
    else if (static_cast<uint32_t>(shaderStage) == 2) // RenderSys::ShaderStage::Fragment
    {
        result = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (static_cast<uint32_t>(shaderStage) == 3) // RenderSys::ShaderStage::Vertex | RenderSys::ShaderStage::Fragment
    {
        result = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (static_cast<uint32_t>(shaderStage) == 4) // RenderSys::ShaderStage::Vertex | RenderSys::ShaderStage::Fragment
    {
        result = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    else
    {
        assert(false);
    }
    
    return result;
}

VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) 
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(GraphicsAPI::Vulkan::GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

VkDescriptorSetLayoutBinding GetVulkanBindGroupLayoutEntry(const RenderSys::BindGroupLayoutEntry& bindGroupLayoutEntry)
{
    VkDescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = bindGroupLayoutEntry.binding;
    assert(layoutBinding.binding >= 0 && layoutBinding.binding <= 4);
    if (bindGroupLayoutEntry.buffer.type == RenderSys::BufferBindingType::Uniform) // A uniform buffer
    {
        if (bindGroupLayoutEntry.buffer.hasDynamicOffset)
        {
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }
        else
        {
            layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
    }
    else if (bindGroupLayoutEntry.buffer.type == RenderSys::BufferBindingType::Storage) // A storage buffer
    {
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (bindGroupLayoutEntry.buffer.type == RenderSys::BufferBindingType::ReadOnlyStorage) // A read-only storage buffer
    {
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (bindGroupLayoutEntry.texture.sampleType != RenderSys::TextureSampleType::Undefined && 
        bindGroupLayoutEntry.texture.viewDimension != RenderSys::TextureViewDimension::Undefined) // A texture
    {
        // use VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE when using two separate bindings for texture and the sampler
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; 
    }
    else if (bindGroupLayoutEntry.sampler.type == RenderSys::SamplerBindingType::Filtering)
    {
        assert(false);
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    else
    {
        assert(false);
    }
    

    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = GetVulkanShaderStageVisibility(bindGroupLayoutEntry.visibility);
    layoutBinding.pImmutableSamplers = nullptr;
    return layoutBinding;
}

VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool) 
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool; 
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(GraphicsAPI::Vulkan::GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool) 
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(GraphicsAPI::Vulkan::GetDeviceQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(GraphicsAPI::Vulkan::GetDeviceQueue()); // Wait for the command buffer to finish

    vkFreeCommandBuffers(GraphicsAPI::Vulkan::GetDevice(), commandPool, 1, &commandBuffer);
}

void TransitionImageLayout(VkImage image, VkFormat format, 
                            VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipMapLevelCount, VkCommandPool commandPool) 
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipMapLevelCount;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } 
    else 
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    EndSingleTimeCommands(commandBuffer, commandPool);
}

std::pair<VkBuffer, VmaAllocation> CreateBuffer(const VmaAllocator& vma, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage) 
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    
    VkBuffer buffer;
    VmaAllocation bufferAllocation;
    if (vmaCreateBuffer(vma, &bufferInfo, &allocInfo, &buffer, &bufferAllocation, nullptr) != VK_SUCCESS) 
    {
        assert(false);
        return std::make_pair(VK_NULL_HANDLE, VK_NULL_HANDLE);
    }

    return std::make_pair(buffer, bufferAllocation);
}

void SetBufferData(const VmaAllocator &vma, VmaAllocation &bufferAllocation, const void *bufferData, VkDeviceSize bufferSize)
{
    assert(bufferData != nullptr);
    void* data;
    vmaMapMemory(vma, bufferAllocation, &data);
    memcpy(data, bufferData, static_cast<size_t>(bufferSize));
    vmaUnmapMemory(vma, bufferAllocation);
}

uint32_t GetUniformStride(const uint32_t sizeOfUniform)
{
    static VkDeviceSize minUniformBufferOffsetAlignment = 0;
    if (minUniformBufferOffsetAlignment == 0)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(GraphicsAPI::Vulkan::GetPhysicalDevice(), &deviceProperties);
        minUniformBufferOffsetAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;
        //std::cout << "maxMemoryAllocationCount : " << deviceProperties.limits.maxMemoryAllocationCount << std::endl;
        //std::cout << "maxDrawIndexedIndexValue : " << deviceProperties.limits.maxDrawIndexedIndexValue << std::endl;
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

} // namespace Vulkan
} // namespace RenderSys
