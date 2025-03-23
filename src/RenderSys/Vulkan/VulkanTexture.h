#pragma once

#include <stdint.h>

namespace RenderSys
{

class VulkanTexture
{
public:
    VulkanTexture(unsigned char* textureData, int width, int height, uint32_t mipMapLevelCount);
    ~VulkanTexture();
private:
    /* data */
};


} // namespace RenderSys