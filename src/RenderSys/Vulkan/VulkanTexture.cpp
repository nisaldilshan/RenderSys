#include "VulkanTexture.h"

#include "VulkanMemAlloc.h"
#include "VulkanRendererUtils.h"

namespace RenderSys
{

VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount)
    : m_image(VK_NULL_HANDLE)
    , m_imageMemory(VK_NULL_HANDLE)
    , m_descriptorImageInfo(VkDescriptorImageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED})
    , m_imageCreateInfo(VkImageCreateInfo{})
{
    m_imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    m_imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    m_imageCreateInfo.extent = {width, height, 1};
    m_imageCreateInfo.mipLevels = mipMapLevelCount;
    m_imageCreateInfo.arrayLayers = 1;
    m_imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    m_imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    m_imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    m_imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    m_imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo imageAllocInfo{};
    imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    const auto res = vmaCreateImage(Vulkan::GetMemoryAllocator(), &m_imageCreateInfo, &imageAllocInfo, &m_image, &m_imageMemory, nullptr);
    if (res != VK_SUCCESS)
    {
        assert(false);
        return;
    }

    m_descriptorImageInfo.sampler = VK_NULL_HANDLE;
    m_descriptorImageInfo.imageView = RenderSys::Vulkan::CreateImageView(m_image, m_imageCreateInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
    m_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VulkanTexture::~VulkanTexture()
{
    if (m_descriptorImageInfo.imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(GraphicsAPI::Vulkan::GetDevice(), m_descriptorImageInfo.imageView, nullptr);
        m_descriptorImageInfo.imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE)
    {
        vmaDestroyImage(RenderSys::Vulkan::GetMemoryAllocator(), m_image, m_imageMemory);
        m_image = VK_NULL_HANDLE;
        m_imageMemory = VK_NULL_HANDLE;
    }
}

void VulkanTexture::SetData(unsigned char *textureData)
{
    // Transition Image to a copyable Layout
    RenderSys::Vulkan::TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, RenderSys::Vulkan::GetCommandPool());

    uint32_t mipLevelWidth = m_imageCreateInfo.extent.width;
    uint32_t mipLevelHeight = m_imageCreateInfo.extent.height;
    uint32_t previousMipLevelWidth = 0;
    std::vector<uint8_t> previousLevelPixels;

    uint32_t mipMapLevelCount = 1; // TODO - Fix mipmap generation (texDescriptor.mipMapLevelCount)
    for (uint32_t level = 0; level < mipMapLevelCount; level++)
    {
        // Create image data
        const auto sizeOfData = 4 * mipLevelWidth * mipLevelHeight;
        std::vector<uint8_t> pixels(sizeOfData);
        if (level == 0)
        {
            memcpy(pixels.data(), textureData, sizeOfData);
        }
        else
        {
            // Create mip level data
            for (uint32_t i = 0; i < mipLevelWidth; ++i)
            {
                for (uint32_t j = 0; j < mipLevelHeight; ++j)
                {
                    uint8_t *p = &pixels[4 * (j * mipLevelWidth + i)];
                    // Get the corresponding 4 pixels from the previous level
                    uint8_t *p00 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelWidth + (2 * i + 0))];
                    uint8_t *p01 = &previousLevelPixels[4 * ((2 * j + 0) * previousMipLevelWidth + (2 * i + 1))];
                    uint8_t *p10 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelWidth + (2 * i + 0))];
                    uint8_t *p11 = &previousLevelPixels[4 * ((2 * j + 1) * previousMipLevelWidth + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                    p[3] = (p00[3] + p01[3] + p10[3] + p11[3]) / 4;
                }
            }
        }

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        std::tie(stagingBuffer, stagingBufferAllocation) = RenderSys::Vulkan::CreateBuffer(RenderSys::Vulkan::GetMemoryAllocator(), static_cast<VkDeviceSize>(pixels.size()),
                                                                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        RenderSys::Vulkan::SetBufferData(RenderSys::Vulkan::GetMemoryAllocator(), stagingBufferAllocation, pixels.data(), static_cast<VkDeviceSize>(pixels.size()));

        auto currentCommandBuffer = RenderSys::Vulkan::BeginSingleTimeCommands(RenderSys::Vulkan::GetCommandPool());

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {mipLevelWidth, mipLevelHeight, 1};

        vkCmdCopyBufferToImage(currentCommandBuffer, stagingBuffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        RenderSys::Vulkan::EndSingleTimeCommands(currentCommandBuffer, RenderSys::Vulkan::GetCommandPool());

        vmaDestroyBuffer(RenderSys::Vulkan::GetMemoryAllocator(), stagingBuffer, stagingBufferAllocation);

        previousLevelPixels = std::move(pixels);
        previousMipLevelWidth = mipLevelWidth;
        mipLevelWidth /= 2;
        mipLevelHeight /= 2;
    }

    // Transition Image to Shader Readable Layout
    RenderSys::Vulkan::TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, RenderSys::Vulkan::GetCommandPool());
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorImageInfo() const
{
    return m_descriptorImageInfo;
}

} // namespace RenderSys