#include "VulkanTexture.h"

#include "VulkanMemAlloc.h"
#include "VulkanRendererUtils.h"
#include "VulkanBuffer.h"
#include <iostream>

namespace RenderSys
{

VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, uint32_t mipMapLevelCount, TextureUsage usage)
    : m_width(width)
    , m_height(height)
    , m_image(VK_NULL_HANDLE)
    , m_imageMemory(VK_NULL_HANDLE)
    , m_descriptorImageInfo(VkDescriptorImageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED})
    , m_imageCreateInfo(VkImageCreateInfo{})
{
    const bool isDepthTexture = (usage == TextureUsage::SAMPLED_UNDEFINED_DEPTHSTENCIL);
    m_imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    m_imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    m_imageCreateInfo.format = isDepthTexture ? Vulkan::GetDepthFormat() : VK_FORMAT_R8G8B8A8_SRGB;
    m_imageCreateInfo.extent = {m_width, m_height, 1};
    m_imageCreateInfo.mipLevels = mipMapLevelCount;
    m_imageCreateInfo.arrayLayers = 1;
    m_imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    m_imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    auto usageFlags = (isDepthTexture ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_TRANSFER_DST_BIT) |
                      VK_IMAGE_USAGE_SAMPLED_BIT;
    m_imageCreateInfo.usage = usageFlags;
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
    m_descriptorImageInfo.imageView = RenderSys::Vulkan::CreateImageView(
        m_image, m_imageCreateInfo.format, isDepthTexture ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT);
    m_descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VulkanTexture::~VulkanTexture()
{
    if (m_descriptorImageInfo.sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(GraphicsAPI::Vulkan::GetDevice(), m_descriptorImageInfo.sampler, nullptr);
        m_descriptorImageInfo.sampler = VK_NULL_HANDLE;
    }
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

        auto stagingBuf = std::make_shared<VulkanBuffer>(pixels.size(), BufferUsage::TRANSFER_SRC_VISIBLE_TO_GPU);
        stagingBuf->MapBuffer();
        stagingBuf->WriteToBuffer(pixels.data());

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
        vkCmdCopyBufferToImage(currentCommandBuffer, stagingBuf->GetBufferInfo().buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        RenderSys::Vulkan::EndSingleTimeCommands(currentCommandBuffer, RenderSys::Vulkan::GetCommandPool());

        previousLevelPixels = std::move(pixels);
        previousMipLevelWidth = mipLevelWidth;
        mipLevelWidth /= 2;
        mipLevelHeight /= 2;
    }

    // Transition Image to Shader Readable Layout
    RenderSys::Vulkan::TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB,
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, RenderSys::Vulkan::GetCommandPool());
}

void VulkanTexture::SetSampler(RenderSys::TextureSampler sampler)
{
    VkSamplerCreateInfo texSamplerInfo{};
    texSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    texSamplerInfo.magFilter = sampler.magFilter == RenderSys::TextureSampler::FilterMode::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    texSamplerInfo.minFilter = sampler.minFilter == RenderSys::TextureSampler::FilterMode::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    texSamplerInfo.addressModeU = sampler.addressModeU == RenderSys::TextureSampler::AddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.addressModeV = sampler.addressModeV == RenderSys::TextureSampler::AddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.addressModeW = sampler.addressModeW == RenderSys::TextureSampler::AddressMode::REPEAT ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    texSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    texSamplerInfo.unnormalizedCoordinates = VK_FALSE;
    texSamplerInfo.compareEnable = VK_FALSE;
    texSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    texSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    texSamplerInfo.mipLodBias = 0.0f;
    texSamplerInfo.minLod = 0.0f;
    texSamplerInfo.maxLod = 0.0f;
    texSamplerInfo.anisotropyEnable = VK_FALSE;
    texSamplerInfo.maxAnisotropy = 1.0f;

    assert(m_descriptorImageInfo.sampler == VK_NULL_HANDLE);
    if (vkCreateSampler(GraphicsAPI::Vulkan::GetDevice(), &texSamplerInfo, nullptr, &m_descriptorImageInfo.sampler) != VK_SUCCESS) {
        std::cout << "error: could not create sampler for texture" << std::endl;
    }
}

const VkDescriptorImageInfo* VulkanTexture::GetDescriptorImageInfoAddr() const
{
    return &m_descriptorImageInfo;
}

} // namespace RenderSys