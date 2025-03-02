#pragma once
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/RenderUtil.h>

namespace GraphicsAPI
{

VkFormat RenderSysFormatToVulkanFormat(RenderSys::VertexFormat format);
std::pair<int, VkDeviceSize> FindAppropriateMemoryType(const VkBuffer& buffer, unsigned int flags);
VkShaderStageFlags GetVulkanShaderStageVisibility(RenderSys::ShaderStage shaderStage);
VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
VkDescriptorSetLayoutBinding GetVulkanBindGroupLayoutEntry(const RenderSys::BindGroupLayoutEntry& bindGroupLayoutEntry);
VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool);

void TransitionImageLayout(VkImage image, VkFormat format, 
                            VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipMapLevelCount, VkCommandPool commandPool);

void CreateBuffer(const VmaAllocator& vma, const void* bufferData, VkDeviceSize bufferSize, 
                    VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                    VkBuffer& buffer, VmaAllocation& bufferAllocation);

uint32_t GetUniformStride(const uint32_t sizeOfUniform);

} // namespace GraphicsAPI