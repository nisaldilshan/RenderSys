#pragma once

#include <stdint.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>
#include <vk_mem_alloc.h>
#include <RenderSys/TextureSampler.h>

#include "RenderSys/Texture.h"

namespace RenderSys
{

class VulkanTexture
{
public:
    VulkanTexture() = delete;
    VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount, RenderSys::TextureUsage usage);
    VulkanTexture(const VulkanTexture&) = delete;
    VulkanTexture& operator=(const VulkanTexture&) = delete;
    VulkanTexture(VulkanTexture&&) = delete;
    VulkanTexture& operator=(VulkanTexture&&) = delete;
    ~VulkanTexture();

    void SetData(unsigned char *textureData);
    void SetSampler(RenderSys::TextureSampler sampler);

    const VkDescriptorImageInfo* GetDescriptorImageInfoAddr() const;
    VkImage GetImage() const { return m_image; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
private:
    uint32_t m_width;
    uint32_t m_height;
    VkImage m_image;
    VmaAllocation m_imageMemory;
    VkDescriptorImageInfo m_descriptorImageInfo;
    VkImageCreateInfo m_imageCreateInfo;
};


} // namespace RenderSys