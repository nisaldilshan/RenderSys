#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RenderSys
{

Texture::Texture(unsigned char* textureData, int width, int height, uint32_t mipMapLevelCount)
    : m_textureData(textureData)
    , m_texWidth(width)
    , m_texHeight(height)
    , m_mipMapLevelCount(mipMapLevelCount)
{}

Texture::~Texture()
{
    //stbi_image_free(m_textureData);  // TODO: stbi_image_free only when loaded from stbi_load
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

namespace fs = std::filesystem;

// Equivalent of std::bit_width that is available from C++20 onward
inline uint32_t bit_width(uint32_t m) {
    if (m == 0) return 0;
    else { uint32_t w = 0; while (m >>= 1) ++w; return w; }
}

Texture* loadTextureRaw(const fs::path &path)
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

std::unique_ptr<Texture> loadTextureUnique(const fs::path &path)
{
    return std::unique_ptr<Texture>(loadTextureRaw(path));
}

std::shared_ptr<Texture> loadTextureShared(const fs::path &path)
{
    return std::shared_ptr<Texture>(loadTextureRaw(path));
}

}