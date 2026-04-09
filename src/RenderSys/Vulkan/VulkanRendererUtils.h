#pragma once
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/Vulkan/VulkanGraphics.h>
#include <RenderSys/RenderUtil.h>
#include <memory>

namespace RenderSys
{
namespace Vulkan 
{

struct RenderTarget
{
    VkImageView view;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
};

inline VkFormat GetDepthFormat()
{
    return VK_FORMAT_D32_SFLOAT;
}

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

uint32_t GetUniformStride(const uint32_t sizeOfUniform);
std::unique_ptr<RenderTarget> CreateRenderTarget(VkImageView imageView, VkSampler sampler);

} // namespace Vulkan
} // namespace RenderSys