#pragma once
#include <filesystem>
#include <stb_image.h>

namespace RenderSys
{

struct TextureDescriptor
{
    unsigned char* data = nullptr;
    int width = 0;
	int height = 0; 
	uint32_t mipMapLevelCount = 0;
    bool useDefaultSampler = true;
};

class Texture
{
public:
    Texture(unsigned char* textureData, int width, int height, uint32_t mipMapLevelCount);
    ~Texture();

    unsigned char* GetData() const;
    int GetWidth() const;
    int GetHeight() const;
    uint32_t GetMipLevelCount() const;
    TextureDescriptor GetDescriptor() const;

private:
    unsigned char* m_textureData = nullptr;
    int m_texWidth = 0;
	int m_texHeight = 0; 
	uint32_t m_mipMapLevelCount = 0;
};

Texture* loadTextureRaw(const std::filesystem::path &path);
std::unique_ptr<Texture> loadTextureUnique(const std::filesystem::path &path);


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

}