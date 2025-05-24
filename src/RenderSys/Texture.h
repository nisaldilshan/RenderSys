#pragma once
#include <filesystem>
#include <stb_image.h>
#include "TextureSampler.h"

namespace RenderSys
{

#if (RENDERER_BACKEND == 1)
class OpenGLTexture;
typedef OpenGLTexture TextureType;
#elif (RENDERER_BACKEND == 2)
class VulkanTexture;
typedef VulkanTexture TextureType;
#elif (RENDERER_BACKEND == 3)
class WebGPUTexture;
typedef WebGPUTexture TextureType;
#else
static_assert(false);
#endif

class Texture
{
public:
    Texture() = delete;
    Texture(unsigned char* textureData, int width, int height, uint32_t mipMapLevelCount);
    Texture(const std::filesystem::path &path);
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
    ~Texture() = default;

    void SetSampler(const TextureSampler& sampler);
    void SetDefaultSampler();
    std::shared_ptr<RenderSys::TextureType> GetPlatformTexture() const;

    static std::shared_ptr<Texture> createDummy(int texWidth, int texHeight);
private:
    int m_texWidth = 0;
	int m_texHeight = 0; 
    std::shared_ptr<RenderSys::TextureType> m_platformTexture;
};

}