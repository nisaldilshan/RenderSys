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

}