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
    : m_texWidth(width)
    , m_texHeight(height)
{
    m_platformTexture = std::make_shared<RenderSys::TextureType>(m_texWidth, m_texHeight, mipMapLevelCount);
    m_platformTexture->SetData(textureData);
}

Texture::Texture(const std::filesystem::path &path)
{
    int channels;
    unsigned char *pixelData = stbi_load(path.string().c_str(), &m_texWidth, &m_texHeight, &channels, 4 /* force 4 channels */);
    if (nullptr == pixelData)
    {
        assert(false);
    }

    const uint32_t mipMapLevelCount = bit_width(std::max(m_texWidth, m_texHeight));
    m_platformTexture = std::make_shared<RenderSys::TextureType>(m_texWidth, m_texHeight, mipMapLevelCount);
    m_platformTexture->SetData(pixelData);
    stbi_image_free(pixelData);
}

void Texture::SetSampler(const TextureSampler &sampler)
{
    if (m_platformTexture)
        m_platformTexture->SetSampler(sampler);
    else
        assert(false);
}

void Texture::SetDefaultSampler()
{
    TextureSampler sampler;
    sampler.minFilter = TextureSampler::FilterMode::LINEAR;
    sampler.magFilter = TextureSampler::FilterMode::LINEAR;
    sampler.addressModeU = TextureSampler::AddressMode::CLAMP_TO_EDGE;
    sampler.addressModeV = TextureSampler::AddressMode::CLAMP_TO_EDGE;
    sampler.addressModeW = TextureSampler::AddressMode::CLAMP_TO_EDGE;
    SetSampler(sampler);
}

std::shared_ptr<RenderSys::TextureType> Texture::GetPlatformTexture() const
{
    return m_platformTexture;
}

}