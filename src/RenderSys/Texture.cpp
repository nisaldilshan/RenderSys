#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#if (RENDERER_BACKEND == 1)
static_assert(false);
#elif (RENDERER_BACKEND == 2)
#include "Vulkan/VulkanTexture.h"
#elif (RENDERER_BACKEND == 3)
#include "WebGPU/WebGPUTexture.h"
#else
static_assert(false);
#endif

namespace RenderSys
{

// Equivalent of std::bit_width that is available from C++20 onward
inline uint32_t bit_width(uint32_t m) {
    if (m == 0) return 0;
    else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

Texture::Texture(unsigned char* textureData, int width, int height, uint32_t mipMapLevelCount)
    : m_textureData(textureData)
    , m_texWidth(width)
    , m_texHeight(height)
    , m_mipMapLevelCount(mipMapLevelCount)
{
    m_platformTexture = std::make_shared<VulkanTexture>(width, height, mipMapLevelCount);
    m_platformTexture->SetData(textureData);
}

Texture::Texture(const std::filesystem::path &path)
{
    int channels;
    int width;
    int height;
    unsigned char *pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
    if (nullptr == pixelData)
    {
        assert(false);
    }

    const uint32_t mipMapLevelCount = bit_width(std::max(width, height));
    m_platformTexture = std::make_shared<VulkanTexture>(width, height, mipMapLevelCount);
    m_platformTexture->SetData(pixelData);
    stbi_image_free(pixelData);
}

unsigned char* Texture::GetData() const
{
    return m_textureData;
}

int Texture::GetWidth() const
{
    return m_texWidth;
}

int Texture::GetHeight() const 
{
    return m_texHeight;
}

uint32_t Texture::GetMipLevelCount() const 
{
    return m_mipMapLevelCount;
}

TextureSampler Texture::GetSampler() const
{
    return m_sampler;
}

void Texture::SetSampler(const TextureSampler &sampler)
{
    m_sampler = sampler;
}

std::shared_ptr<RenderSys::TextureType> Texture::GetPlatformTexture() const
{
    return m_platformTexture;
}

Texture* loadTextureRaw(const std::filesystem::path &path)
{
    int channels;
    int width;
    int height;
    unsigned char *pixelData = stbi_load(path.string().c_str(), &width, &height, &channels, 4 /* force 4 channels */);
    if (nullptr == pixelData)
    {
        assert(false);
        return nullptr;
    }

    uint32_t mipMapLevelCount = bit_width(std::max(width, height));
    return new Texture(pixelData, width, height, mipMapLevelCount);
}

std::unique_ptr<Texture> loadTextureUnique(const std::filesystem::path &path)
{
    return std::unique_ptr<Texture>(loadTextureRaw(path));
}

std::shared_ptr<Texture> loadTextureShared(const std::filesystem::path &path)
{
    return std::shared_ptr<Texture>(loadTextureRaw(path));
}

}