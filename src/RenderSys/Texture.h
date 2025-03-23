#pragma once
#include <filesystem>
#include <stb_image.h>

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

enum class SamplerAddressMode
{
    REPEAT = 0,
    CLAMP_TO_EDGE,
    MIRRORED_REPEAT
};

enum class SamplerFilterMode
{
    NEAREST = 0,
    LINEAR,
};

struct TextureSampler 
{
    SamplerFilterMode magFilter;
    SamplerFilterMode minFilter;
    SamplerAddressMode addressModeU;
    SamplerAddressMode addressModeV;
    SamplerAddressMode addressModeW;
};

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
    ~Texture();

    unsigned char* GetData() const;
    int GetWidth() const;
    int GetHeight() const;
    uint32_t GetMipLevelCount() const;
    TextureSampler GetSampler() const;
    void SetSampler(const TextureSampler& sampler);

private:
    unsigned char* m_textureData = nullptr;
    int m_texWidth = 0;
	int m_texHeight = 0; 
	uint32_t m_mipMapLevelCount = 0;
    TextureSampler m_sampler;
    std::shared_ptr<RenderSys::TextureType> m_platformTexture;
};

Texture* loadTextureRaw(const std::filesystem::path &path);
std::unique_ptr<Texture> loadTextureUnique(const std::filesystem::path &path);
std::shared_ptr<Texture> loadTextureShared(const std::filesystem::path &path);


}