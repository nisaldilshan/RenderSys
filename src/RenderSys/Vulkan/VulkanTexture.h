#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <vk_mem_alloc.h>

namespace RenderSys
{

class VulkanTexture
{
public:
    VulkanTexture() = delete;
    VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount);
    ~VulkanTexture();
    void SetData(unsigned char *textureData);
private:
    VkImage m_image;
    VmaAllocation m_imageMemory;
    VkDescriptorImageInfo m_descriptorImageInfo;
    VkImageCreateInfo m_imageCreateInfo;
};


} // namespace RenderSys