#include "VulkanTexture.h"

#include "VulkanMemAlloc.h"
#include "VulkanRendererUtils.h"

namespace RenderSys
{

VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipMapLevelCount;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imageAllocInfo;
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(Vulkan::GetMemoryAllocator(), &imageInfo, &imageAllocInfo, &m_image, &m_imageMemory, nullptr) != VK_SUCCESS) 
    {
        assert(false);
        return;
    }

    m_descriptorImageInfo.imageView = RenderSys::Vulkan::CreateImageView(m_image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
    m_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VulkanTexture::~VulkanTexture()
{
}

bool VulkanTexture::SetData(unsigned char *textureData)
{
    return false;
}

} // namespace RenderSys