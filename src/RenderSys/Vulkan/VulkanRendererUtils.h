#pragma once

namespace GraphicsAPI
{

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
    vkGetBufferMemoryRequirements(Vulkan::GetDevice(), buffer, &mem_reqs);

    VkPhysicalDeviceMemoryProperties gpu_mem;
    vkGetPhysicalDeviceMemoryProperties(Vulkan::GetPhysicalDevice(), &gpu_mem);

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
    if (vkCreateImageView(Vulkan::GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create image view!");
    }

    return imageView;
}

VkDescriptorSetLayoutBinding GetVulkanBindGroupLayoutEntry(const RenderSys::BindGroupLayoutEntry& bindGroupLayoutEntry)
{
    VkDescriptorSetLayoutBinding layoutBinding;
    layoutBinding.binding = bindGroupLayoutEntry.binding;
    assert(layoutBinding.binding >= 0 && layoutBinding.binding <= 1);
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
    else if (bindGroupLayoutEntry.texture.sampleType != RenderSys::TextureSampleType::Undefined && 
        bindGroupLayoutEntry.texture.viewDimension != RenderSys::TextureViewDimension::Undefined) // A texture
    {
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;//VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
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

}