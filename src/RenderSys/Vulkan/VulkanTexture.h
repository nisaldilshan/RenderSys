#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <vk_mem_alloc.h>

namespace RenderSys
{

class VulkanTexture
{
public:
    VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount);
    ~VulkanTexture();
    bool SetData(unsigned char *textureData);
private:
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_imageMemory = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_descriptorImageInfo = VkDescriptorImageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
};


} // namespace RenderSys