#pragma once
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <RenderSys/RenderUtil.h>

namespace RenderSys
{
namespace Vulkan 
{

void CreateCommandPool();
VkCommandPool GetCommandPool();
void DestroyCommandPool();

VkFormat RenderSysFormatToVulkanFormat(RenderSys::VertexFormat format);
std::pair<int, VkDeviceSize> FindAppropriateMemoryType(const VkBuffer& buffer, unsigned int flags);
VkShaderStageFlags GetVulkanShaderStageVisibility(RenderSys::ShaderStage shaderStage);
VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
VkDescriptorSetLayoutBinding GetVulkanBindGroupLayoutEntry(const RenderSys::BindGroupLayoutEntry& bindGroupLayoutEntry);
VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool);
void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool);

void TransitionImageLayout(VkImage image, VkFormat format, 
                            VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipMapLevelCount, VkCommandPool commandPool);

std::pair<VkBuffer, VmaAllocation> CreateBuffer(const VmaAllocator& vma, const VkDeviceSize bufferSize, const VkBufferUsageFlags usage, const VmaMemoryUsage memoryUsage);
void SetBufferData(const VmaAllocator& vma, VmaAllocation& bufferAllocation, const void* bufferData, VkDeviceSize bufferSize);

uint32_t GetUniformStride(const uint32_t sizeOfUniform);

} // namespace Vulkan
} // namespace RenderSys