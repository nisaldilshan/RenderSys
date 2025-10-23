#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

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

Texture::Texture(int width, int height, uint32_t mipMapLevelCount, TextureUsage usage)
    : m_texWidth(width)
    , m_texHeight(height)
{
    m_platformTexture = std::make_shared<RenderSys::TextureType>(m_texWidth, m_texHeight, mipMapLevelCount, usage);
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
    m_platformTexture = std::make_shared<RenderSys::TextureType>(m_texWidth, m_texHeight, mipMapLevelCount, TextureUsage::SAMPLED_TRANSFERDST_UNDEFINED);
    m_platformTexture->SetData(pixelData);
    stbi_image_free(pixelData);
}

void Texture::SetData(unsigned char *textureData)
{
    m_platformTexture->SetData(textureData);
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

std::shared_ptr<Texture> Texture::createDummy(int texWidth, int texHeight)
{
    std::vector<uint8_t> pixels(4 * texWidth * texHeight);

    for (uint32_t i = 0; i < texWidth; ++i) {
        for (uint32_t j = 0; j < texHeight; ++j) {
            uint8_t *p = &pixels[4 * (j * texWidth + i)];
            p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
            p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
            p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
            p[3] = 255; // a
        }
    }

    auto dummyTexture = std::make_shared<RenderSys::Texture>(texWidth, texHeight, 1,
                                                             TextureUsage::SAMPLED_TRANSFERDST_UNDEFINED);
    dummyTexture->SetData(pixels.data());
    dummyTexture->SetDefaultSampler();
    return dummyTexture;
}

std::shared_ptr<Texture> Texture::createDepthDummy(int texWidth, int texHeight)
{
    auto dummyTexture = std::make_shared<RenderSys::Texture>(texWidth, texHeight, 1,
                                                             TextureUsage::SAMPLED_UNDEFINED_DEPTHSTENCIL);
    dummyTexture->SetDefaultSampler();
    return dummyTexture;
}

}